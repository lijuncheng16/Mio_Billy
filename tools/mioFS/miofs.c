/*
 * mio_fuse.c
 *
 *  Created on: 10 de Mar de 2014
 *      Author: Ricardo Lopes Pereira
 */

#define MIOFS_VERSION "0.1"

#define FUSE_USE_VERSION 30
//#define _POSIX_SOURCE  // for using strtok_r

#include <fuse.h>
#include <fuse_opt.h>
#include <errno.h>

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <uthash.h>
#include <utlist.h>

#include <mio.h>
#include "mio_client.h"

#define FOLDER_DELIMITER "/"
#define DATA_FRESH_TIMEOUT 300	// default value (in seconds) for data staleness
/* From mio_client.c */
//TODO this variable could be set in the fuse private structure which can be accessed using getcontext() Maybe this way is more explicit
/* hash table with all the nodes. Is also a tree for collection nodes */
extern struct mio_node *fsRoot;

/******************************************************
 * FUSE operations
 ******************************************************/

static struct mio_node* getHandleFromPath(const char* _path) {
	char *path = strdup(_path);
	char *folder, *aux = NULL;
	struct mio_node *handle = fsRoot;
	struct node_child *lres;

	//go through the path, one folder at a time
	folder = strtok_r(path, FOLDER_DELIMITER, &aux);
	if (folder)
		do {
			// Update collection nodes children. Root node is collection node. While ensures all others will be collection nodes
			if (update_children(handle))
				break;

			// search for this name in the children of the current node
			LL_SEARCH(handle->children, lres, folder, cmp_node_child_str);

			if (!lres) {
				handle = NULL;
				break;
			} else
				handle = lres->child;
		} while ((folder = strtok_r(NULL, FOLDER_DELIMITER, &aux))
				&& handle->type == MIO_NODE_TYPE_COLLECTION);

	free(path);
	// did it reach end of path?
	if (folder)
		return NULL ;
	else
		return handle;
}

/*
 * Operations for managing directories.
 */

/* Open dir. Files the fi->fh with a pointer to the structure for the directory in _path */
static int miofs_opendir(const char *path, struct fuse_file_info *fi) {
	struct mio_node *handle;

	handle = getHandleFromPath(path);
	// make sure data is current
	if (update_children(handle))
		return -ENOENT;
	if (handle) {
		fi->fh = (uint64_t) handle;
		return 0;
	} else {
		return -ENOENT;
	}
}

static int miofs_releasedir(const char *path, struct fuse_file_info *fi) {
	return 0;
}

static int miofs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		off_t offset, struct fuse_file_info *fi) {
	struct mio_node *handle = (struct mio_node *) fi->fh;
	struct node_child *child_n;
//	printf("aceder %s\n", path);

	if (!handle)
		return -ENOENT;

	// TODO set attributes for files?

	// set access time
	get_current_time(&(handle->access_time));

	// fill in file names
	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);
	LL_FOREACH( handle->children, child_n)
	{
		filler(buf, child_n->child->id, NULL, 0);
//		printf("* %s\n", child_n->child->id);
	}

	return 0;
}

static int miofs_getattr(const char *path, struct stat *stbuf) {
	struct mio_node *handle = getHandleFromPath(path);
	struct node_child *l_aux;

	printf("getattr %s\n", path);
	memset(stbuf, 0, sizeof(struct stat));
	if (!handle)
		return -ENOENT;

	// define file modification and access time
#ifdef __MACH__
	stbuf->st_mtimespec = handle->modification_time;
	stbuf->st_atimespec = handle->access_time;
#else	
	stbuf->st_mtim = handle->modification_time;
	stbuf->st_atim = handle->access_time;
#endif	
	//TODO obtain parents in order to define number of links
	switch (handle->type) {
	case MIO_NODE_TYPE_COLLECTION:
		update_children( handle);
		stbuf->st_mode = S_IFDIR | 0555;
		stbuf->st_nlink = 2;
		LL_COUNT( handle->children, l_aux, stbuf->st_size);
		break;
	default:
	    update_node_data( handle);
		stbuf->st_mode = S_IFREG | 0444;
		stbuf->st_nlink = 1;
		stbuf->st_size = handle->data ? strlen(handle->data) : 0;
	}
	return 0;
}

static int miofs_open(const char *path, struct fuse_file_info *fi) {
	struct mio_node *handle = getHandleFromPath(path);

	// TODO should we subscribe nodes while files are open?

	if (handle) {
		fi->fh = (long) handle;
		// update file data
		update_node_data(handle);
		return 0;
	} else
		return -ENOENT;

	// TODO verify permissions

//	if (strcmp(path, hello_path) != 0)
//		return -ENOENT;
//	if ((fi->flags & 3) != O_RDONLY)
//		return -EACCES;
//	return 0;
}

static int miofs_read(const char *path, char *buf, size_t size, off_t offset,
		struct fuse_file_info *fi) {
	struct mio_node *handle = (struct mio_node *) fi->fh;
	size_t len;

	if (!handle)
		return -ENOENT;

	// set access time
	get_current_time(&(handle->access_time));

	len = handle->data ? strlen(handle->data) : 0;
	if (offset < len) {
		if (offset + size > len)
			size = len - offset;
		memcpy(buf, handle->data + offset, size);
	} else
		size = 0;
	return size;
}

static int miofs_write (const char *path, const char *buf, size_t len, off_t offset,
		      struct fuse_file_info *fi) {
	struct mio_node *handle = (struct mio_node *) fi->fh;
	char *transducer;
	char *data;

	printf("write %ld B to %s\n", len, path);

	if (!handle)
			return -ENOENT;

	// limit writes to the lutron lights
	if( !strcmp( handle->id, "237b02a8008a3fa30bbfb8114442abc4_data"))
		transducer = "4899";
	else if( !strcmp( handle->id, "688185e424d2eda7d4341167440a2f0f_data"))
		transducer = "4907";
	else if( !strcmp( handle->id, "6facad2d487fcdabc07a6d948a89cd95_data"))
			transducer = "4915";
	else
		return -ENOENT;

	data = strndup( buf + offset, len);
	printf("will actuate %s, %s, %s\n", handle->id, transducer, data);
	actuate( handle, transducer, data);
	free( data);
	return len;
}

static int miofs_truncate( const char *path, off_t size) {
	printf("truncate to %ld B file %s\n", size, path);
	return 0;
}

static int miofs_ftruncate( const char *path, off_t size) {
	printf("ftruncate to %ld B file %s\n", size, path);
	return 0;
}

// TODO file close

static struct fuse_operations miofs_functions = { .getattr = miofs_getattr,
		.opendir = miofs_opendir, .releasedir = miofs_releasedir, .readdir =
				miofs_readdir, .open = miofs_open, .read = miofs_read, .write=miofs_write, .truncate=miofs_truncate, .ftruncate=miofs_ftruncate };

/******************************************************
 * Command line arguments parsing using fuse_opt.h
 * http://sourceforge.net/apps/mediawiki/fuse/index.php?title=Option_parsing
 ******************************************************/
struct miofs_args {
	char *username;
	char *password;
	char *base_collection;
	char *mount_point;
	char debug_mio;
	char debug_miofs;
	int timeout;
};

enum {
	CMD_ARG_HELP, CMD_ARG_VERBOSE, CMD_ARG_VERSION
};

#define MIOFS_ARGS(arg, field, value) { arg, offsetof(struct miofs_args, field), value }

static struct fuse_opt miofs_opts[] = { MIOFS_ARGS("-u %s", username, 0),
		MIOFS_ARGS("-p %s", password, 0),
		MIOFS_ARGS("-c %s", base_collection, 0),
		MIOFS_ARGS("-Df", debug_miofs, 0), MIOFS_ARGS("-Dm", debug_mio, 1),
		MIOFS_ARGS("-t %d", timeout, DATA_FRESH_TIMEOUT),
		FUSE_OPT_KEY("-verbose", CMD_ARG_VERBOSE),
		FUSE_OPT_KEY("-help", CMD_ARG_HELP), FUSE_OPT_KEY("-h", CMD_ARG_HELP),
		FUSE_OPT_KEY("-version", CMD_ARG_VERSION),
		FUSE_OPT_KEY("-v", CMD_ARG_VERSION), FUSE_OPT_END };

static int miofs_opt_proc(void *data, const char *arg, int key,
		struct fuse_args *outargs) {
	switch (key) {
	case CMD_ARG_VERSION:
		fprintf(stderr, "miofs version %s\n", MIOFS_VERSION);
		fuse_opt_add_arg(outargs, "--version");
		fuse_main(outargs->argc, outargs->argv, &miofs_functions, NULL);
		exit(0);
	case CMD_ARG_HELP:
		fprintf(stderr,
				"miofs - Mortr.io (MIO) File System. A FUSE interface for Mortr.io\n"
						"Usage: %s mountpoint [FUSE options] -u <username> -p <password> [MIO options] <mount point>\n"
						"\n"
						"MIO options:\n"
						"    -u <username>  The complete JID of the user, including the domain, from which the XMPP server is obtained\n"
						"    -p <password>  The password for the username JID\n"
						"    -c <JID>       JID (without domain) of a Collection node to be used as root. If not supplied, user's subscriptions are used instead.\n"
						"    -Df            Turn on debug for miofs operations\n"
						"    -Dm            Turn on debug for Mortr.io operation\n"
						"	 -t	<timeout>	Data freshness timeout. Default is %d\n"
						"\n", outargs->argv[0], DATA_FRESH_TIMEOUT);
		fuse_opt_add_arg(outargs, "-ho");
		fuse_main(outargs->argc, outargs->argv, &miofs_functions, NULL);
		exit(1);
//	case FUSE_OPT_KEY_OPT:
//		fprintf( stderr, "Unknown fuse option: %s. Please use -h for help\n", arg);
//		return 1;
	case FUSE_OPT_KEY_NONOPT:
		if (!((struct miofs_args*) data)->mount_point) {
			((struct miofs_args*) data)->mount_point = strdup(arg);
			return 1;
		} else {
			fprintf(stderr,
					"Mount point already set to %s. Unknown parameter %s\n",
					((struct miofs_args*) data)->mount_point, arg);
			exit(1);
		}
	default:
		// ignore other options and pass them on to fuse
		return 1;
	}
}

int main(int argc, char *argv[]) {
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
	struct miofs_args cmd_args;
	int err = 0;

	// parse command line arguments
	memset(&cmd_args, 0, sizeof(cmd_args));
	// this should not be necessary! fuse_opt_parse should set the default values!
	cmd_args.timeout = DATA_FRESH_TIMEOUT;
	fuse_opt_parse(&args, &cmd_args, miofs_opts, miofs_opt_proc);
	if (cmd_args.username == NULL ) {
		fprintf(stderr, "No username supplied. Use -h for help.\n");
		exit(1);
	} else if (cmd_args.password == NULL ) {
		fprintf(stderr, "No password supplied. Use -h for help.\n");
		exit(1);
	}

	err = mio_client_init(cmd_args.debug_mio, cmd_args.username,
			cmd_args.password, cmd_args.timeout);

	if (!err)
		// TODO default behaviour is multithread. Make sure code is thread safe or force single thead with parameter?
		err = fuse_main(args.argc, args.argv, &miofs_functions, NULL);
	else
		fprintf(stderr, "Unable to init MIO\n");

	// clear MIO before terminating
	mio_cleanup();

	// get last data -> permite ir buscar o último valor
	// aqueles onde estamos subscriptos actualizam automáticamente. Os outros não, mas apenas polling

	fprintf(stderr, "Bye!\n");
	return err;

	// TODO implement unique handles per file. This way files can keep growing

	// TODO currently does not work (blocks?) if launched without the -f option. Check it out. printfs?

	//TODO multithreading
	// implement read/write semaphores for the data structures shared with mio_client.c
	// mio_client already uses another thread for receiving data from subscribed nodes!!!

	//TODO better error logging. syslog?

	// TODO support ACL? using whitelists and blacklists xep-0248. NO ACL SUPPORT ON FUSE
	// Check operation result in order to return a permission error when mio operation is not allowed

}

