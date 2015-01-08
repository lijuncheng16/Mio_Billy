#ifndef _MIO_H_
#define _MIO_H_

/* List element used to store subscribed collection nodes, subscribed non-collection nodes and a collection node's children */
struct node_child {
	struct mio_node* child;
	struct node_child *next;
};


/* Hash table element used to store known nodes */
struct mio_node {
	char *id;
	mio_node_type_t type;
	char is_subscribed; // data nodes will be automatically updated as events arrive. Directories will be updated automatically???
	char is_miofs_artifact;
	struct timespec access_time;
	struct timespec modification_time;
	// permissions?
	char *data;
	struct node_child *children;
	UT_hash_handle hh;
};

int cmp_node_child_str( const struct node_child *a, char *b);
void get_current_time(struct timespec *ts);
int mio_client_init( char debug_level, char *username, char *password, int timeout);
void mio_cleanup();
int update_children(struct mio_node *node);
int update_node_data(struct mio_node *node);

#endif
