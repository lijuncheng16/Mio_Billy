/* picoweb - static content web server
   written by maxim buevich
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <math.h>
#include <signal.h>

#include "tile_server.h"

#define LISTENQ   1024
#define BUFLEN    16384
#define STDLEN    128
#define MASTER_BUFLEN    (128 * 1024)

typedef struct sockaddr SA;


void set_docroot(int argc, char **argv, char *docroot);
int open_listenfd(int port);
ssize_t server_read(int fd, char *buf, int len);
ssize_t server_write(int fd, const char *buf, int len);
void send_headers(int fd, char *file_name, char *buf);
const char *get_ftype(char *filename);
void get_request(int fd, char *reqbuf);
void get_file(char **fnamepnt, char *reqbuf);
void send_404(int fd);

void send_json_headers(int fd, char *buf, int size);
void send_json_http(int fd, char *header_buf, char *data_buf, int data_size);

#define N_HTTP_ARGS 10
int n_http_args, cmd_type;
char http_args[N_HTTP_ARGS][STDLEN];

#define N_CMD 3
const char tiling_req[N_CMD][STDLEN] = {"gettile", "info.json", "tiles"};
const char tiling_cmd[N_CMD][STDLEN] = {DATASTORE_DIR"/gettile", DATASTORE_DIR"/info"};
const char import_cmd[] = DATASTORE_DIR"/import";
/*#define KVS_DIR "../store.kvs"*/ // moved to tile_server.h
#define USER_ID "1"
#define GETTILE_OUTPUT "/tmp/tile.server.output.json"
#define MASTER_OUTPUT "/tmp/master.server.output.json"


int is_tiling_request(char *req)
{
  unsigned int ptr_now = 0, ptr_prev = 0, i;
  char *pos;
  cmd_type = -1;
  if (*req == '/') ptr_now++;
  for (i = 0; i < N_CMD; i++) { 
    pos = req + ptr_now;
    if (strncmp(tiling_req[i], pos, strlen(tiling_req[i])) == 0) {
      /* .json */
      if (i == 2) {
        pos = req + strlen(req) - strlen(".json");
        if (strncmp(".json", pos, strlen(".json"))) continue;
        *pos = 0;
      }
      cmd_type = i;
      break;
    }
  }
  if (cmd_type < 0) return 0;

  /* split request command */
  n_http_args = 0;
  ptr_prev = ptr_now;
  while (ptr_now < strlen(req)) {
    if (req[ptr_now] == '+' || req[ptr_now] == '/') {
      strncpy(http_args[n_http_args], req + ptr_prev, ptr_now - ptr_prev);
      http_args[n_http_args++][ptr_now - ptr_prev] = 0;
      ptr_prev = ptr_now + 1;
      if (n_http_args >= N_HTTP_ARGS) break;
    }
    ptr_now++;
  }
  if (ptr_now != ptr_prev && n_http_args < N_HTTP_ARGS) {
    strncpy(http_args[n_http_args], req + ptr_prev, ptr_now - ptr_prev);
    http_args[n_http_args++][ptr_now - ptr_prev] = 0;
  }
  if (cmd_type == 2 && n_http_args < N_HTTP_ARGS) {
    pos = http_args[n_http_args - 1];
    while (*pos && *pos != '.') pos++;
    if (*pos == '.') {
      *pos = 0;
      strcpy(http_args[n_http_args++], pos + 1);
    }
  }
  return 1;
}

#define SLAVE_LIST "/slave.json"
#define MASTER_PERIOD 10
//#define MASTER_KVS "../master.kvs"
#define MASTER_KVS KVS_DIR

#include <fstream>
#include <string>
#include <json/json.h>

void run_master_task(int argc, char *argv[])
{
  char buf[STDLEN];
  char sendbuf[MASTER_BUFLEN], recvbuf[MASTER_BUFLEN];

  set_docroot(argc,argv,buf);
  std::string filename = buf;
  filename.append(SLAVE_LIST);
  unsigned int i;
  int port = atoi(argv[1]);

  Json::Reader reader;
  std::ifstream instream(filename.c_str());
  Json::Value slave_json;
  if (!reader.parse(instream, slave_json)) {
    printf("failed to read %s\n", filename.c_str());
    return;
  }
  Json::Value slaves = slave_json["slave"];
  
  
  while (1) {
    Json::Value kvs_json;
    sprintf(buf, "%s %s -r %s 2> /dev/null > %s", tiling_cmd[1], MASTER_KVS, USER_ID, MASTER_OUTPUT);
    int ret = system(buf);

    std::ifstream kvs_instream(MASTER_OUTPUT);
    if (!reader.parse(kvs_instream, kvs_json)) {
      printf("failed to read %s\n", MASTER_OUTPUT);
      sleep(MASTER_PERIOD);
      continue;
    }
    Json::Value channel_json = kvs_json["channel_specs"];
    //double min_time = kvs_json["min_time"].asDouble();
    double max_time = kvs_json["max_time"].asDouble();

    Json::Value::Members slave_members = slaves.getMemberNames();
    for (i = 0; i < slave_members.size(); i++) {
    //for (i = 0; i < 1; i++) {
      //Json::Value slave_sensor_info = channel_json[slave_info[(unsigned int)0].asString().c_str()];
      //Json::Value slave_sensor_info = channel_json["ff1.motion"];
      //Json::Value slave_channel_bound = slave_sensor_info["channel_bounds"];
      //printf("----- %lf\n", slave_channel_bound["min_time"].asDouble());

      printf("Tile migration - %s / %s\n", slave_members[i].c_str(), slaves[slave_members[i]].asString().c_str());
      //printf("%d : %s / %s\n", i, slave_info[(unsigned int)0].asString().c_str(), slave_info[(unsigned int)1].asString().c_str());

      // send info request
      int sock;
      if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        printf("cannot setup socket...\n");
        continue;
      }
      struct sockaddr_in addr;
      memset(&addr, 0, sizeof(addr));
      addr.sin_family = AF_INET;
      addr.sin_addr.s_addr = inet_addr(slaves[slave_members[i]].asString().c_str());
      addr.sin_port = htons(port);

      if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        printf("connect failed...\n");
        continue;
      }

      sprintf(sendbuf, "GET /info.json HTTP/1.1\r\n\
                        Host: %s:%d\r\n\
                        Connection: keep-alive\r\n\
                        User-Agent: Mozilla/5.0\r\n", 
                        slaves[slave_members[i]].asString().c_str(), port);
      server_write(sock, sendbuf, strlen(sendbuf));
      int recv_len = 0;
      while ((ret = server_read(sock, recvbuf + recv_len, MASTER_BUFLEN - recv_len)) > 0) {
        recv_len += ret;
	if (recv_len >= MASTER_BUFLEN) break;
      }
      close(sock);

      int start_ptr = -1;
      while (start_ptr < recv_len && recvbuf[++start_ptr] != '{');

      Json::Value info_json;
      if (!reader.parse(recvbuf+start_ptr, recvbuf+recv_len, info_json)) {
        continue;
      }
      //Json::Value info_channel_json = info_json["channel_specs"];
      double slave_min_time = info_json["min_time"].asDouble();
      double slave_max_time = info_json["max_time"].asDouble();

      long long req_level = (long long)(log((slave_max_time - slave_min_time) / 512) / log(2)) - 2;
      //long long req_time_duration = pow(2, req_level) * 512;
      double req_start_time;
      req_start_time = max_time;
      if (req_start_time <= 0) req_start_time = slave_min_time;
      if (req_start_time > slave_max_time) continue;

      long long req_start_offset = (long long)(req_start_time / 512 / pow(2, req_level));
      long long req_end_offset = (long long)(slave_max_time / 512 / pow(2, req_level));

      //printf("-> %s \n-> %lf, %lf\n\n", recvbuf + start_ptr, slave_min_time, slave_max_time);
      //printf("-> level:%lld, start:%lld, end:%lld\n", req_level, req_start_offset, req_end_offset);

      // send gettile request to slave
      for (; req_start_offset <= req_end_offset; req_start_offset++) {
        if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
          printf("cannot setup socket(2)...\n");
          continue;
        }
        if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
          printf("connect(2nd) failed...\n");
          continue;
        }
  
        sprintf(sendbuf, "GET /tiles/%s/%s/%lld.%lld.json HTTP/1.1\r\n\
                          Host: %s:%d\r\n\
                          Connection: keep-alive\r\n\
                          User-Agent: Mozilla/5.0\r\n", 
  			  USER_ID, slave_members[i].c_str(), req_level, req_start_offset,
                          slaves[slave_members[i]].asString().c_str(), port);
	//printf("%s", sendbuf);
        server_write(sock, sendbuf, strlen(sendbuf));
        recv_len = 0;
        while ((ret = server_read(sock, recvbuf + recv_len, MASTER_BUFLEN - recv_len)) > 0) {
          recv_len += ret;
  	  if (recv_len >= MASTER_BUFLEN) break;
        }
        close(sock);
        
	recvbuf[recv_len] = 0;
	start_ptr = -1;
        while (start_ptr < recv_len && recvbuf[++start_ptr] != '{');

        Json::Value gettile_json;
        if (!reader.parse(recvbuf+start_ptr, recvbuf+recv_len, gettile_json)) {
          continue;
        }

	/*
	int tmp_ptr = start_ptr;
        while (tmp_ptr < recv_len) {
	  if (recvbuf[tmp_ptr] == '\'') recvbuf[tmp_ptr] = '\"';
	  tmp_ptr++;
	}*/

	std::string slave_dev_name = slave_members[i].c_str();
	std::string slave_channel_name = slave_members[i].c_str();
	slave_dev_name = slave_dev_name.substr(0, slave_dev_name.find_first_of("."));
	slave_channel_name = slave_channel_name.substr(slave_channel_name.find_first_of(".") + 1, slave_channel_name.length());
        
	FILE *fp;
	fp = fopen(MASTER_OUTPUT, "w");
	fprintf(fp, "{\"user_id\": \"%s\", \"channel_names\": [\"%s\"], \"dev_nickname\": \"%s\", \"controller\": \"logrecs\", \"action\": \"upload\", \"data\":[\n",
	  USER_ID, slave_channel_name.c_str(), slave_dev_name.c_str());
        //if (start_ptr < recv_len) fprintf(fp, "%s", recvbuf + start_ptr);
	//else fprintf(fp, "}");
   for (unsigned int j = 0; j < gettile_json["data"].size(); j++) {
      if(gettile_json["data"][j][(unsigned int)1].asDouble() <= -1e308){
         if(j == 0)
            fprintf(fp, "[%#.16g, %#.16g]", gettile_json["data"][1][(unsigned int)0].asDouble(), gettile_json["data"][1][(unsigned int)1].asDouble());
         continue;
      }
      else if (j == 0) fprintf(fp, "[%#.16g, %#.16g]", gettile_json["data"][j][(unsigned int)0].asDouble(), gettile_json["data"][j][(unsigned int)1].asDouble());
      else fprintf(fp, ",\n[%#.16g, %#.16g]", gettile_json["data"][j][(unsigned int)0].asDouble(), gettile_json["data"][j][(unsigned int)1].asDouble());
   }
   fprintf(fp, "\n], \"method\": \"post\"}");
	fclose(fp);

        sprintf(buf, "%s %s %s %s %s 2> /dev/null", import_cmd, MASTER_KVS, USER_ID, slave_dev_name.c_str(), MASTER_OUTPUT);
	//printf("%s\n", buf);
        ret = system(buf);
      }
      
    }
    sleep(MASTER_PERIOD);
  }
}

int run_tile_server(int argc, char *argv[]){
  int listenfd;
  int connfd;
  struct sockaddr_in clientaddr;
  unsigned int clientlen;
  int datalen;
  
  char docroot[STDLEN];
  char *databuf;
  char filename[8*STDLEN];
  char *fnamepnt = filename;
  char filepath[STDLEN];
  FILE *file;
  int ret;

  if((listenfd = open_listenfd(atoi(argv[1]))) < 0){
    printf("bad port number. try a port larger than 1023.\n");
    exit(0);
  }
  
  set_docroot(argc,argv,docroot);
  printf("running...\n"); fflush(stdout);
  clientlen = sizeof(clientaddr);
  databuf = (char*)malloc(BUFLEN);
  
  while(1){
    if((connfd = accept(listenfd,(SA *)&clientaddr,&clientlen)) < 0)
      return -1;
    get_request(connfd,filename);
    get_file(&fnamepnt,filename);
    /*
	 while(server_read(connfd,databuf,BUFLEN)==BUFLEN)
      ;
		*/
    
    if(fnamepnt[strlen(fnamepnt)-1]=='/') {
      sprintf(fnamepnt,"%sindex.html",fnamepnt);
    }
    if (!is_tiling_request(fnamepnt)) {
      sprintf(filepath,"%s%s",docroot,fnamepnt);
      file = fopen(filepath,"r");
  
      if(!file){
        send_404(connfd);
        printf("[%s]: 404 NOT FOUND\n",fnamepnt); fflush(stdout);
      }
      else{
        printf("[%s]\n",fnamepnt); fflush(stdout);
        send_headers(connfd,filepath,databuf);
        while((datalen = fread(databuf,1,BUFLEN,file)))
          server_write(connfd,databuf,datalen);
        fclose(file);
      }
    }
    else {
      printf("tiling request! %s cmd:%d\n", fnamepnt, cmd_type);
      if (cmd_type == 0) { /* gettile */
	gettile(atoi(USER_ID), http_args[1], atoi(http_args[2]), atoll(http_args[3]), connfd, databuf);
      }
      else if (cmd_type == 1) { /* info */
        sprintf(databuf, "%s %s -r %s 2> /dev/null > %s", tiling_cmd[cmd_type], KVS_DIR, USER_ID, GETTILE_OUTPUT);
        printf("CMD : %s\n", databuf);
        ret = system(databuf);
        sprintf(filepath, "%s", GETTILE_OUTPUT);

	// file read and send
        file = fopen(filepath,"r");
    
        if(!file){
          send_404(connfd);
          printf("[%s]: 404 NOT FOUND\n",fnamepnt); fflush(stdout);
        }
        else{
          printf("[%s]\n",fnamepnt); fflush(stdout);
          send_headers(connfd,filepath,databuf);
          while((datalen = fread(databuf,1,BUFLEN,file)))
            server_write(connfd,databuf,datalen);
          fclose(file);
        }
      }
      else if (cmd_type == 2) { /* gettile (.json) */
        gettile(atoi(http_args[1]), http_args[2], atoi(http_args[3]), atoll(http_args[4]), connfd, databuf);
      }
    }

    close(connfd);
  }
  return 0;
}

int main(int argc, char *argv[]){
  if(argc<2 || argc>4){
    printf("usage: %s PORT [DOCROOT] [0:Slave/1:Master]\n", argv[0]);
    exit(0);
  }

  signal(SIGPIPE, SIG_IGN);

  if (argc > 3 && atoi(argv[3]) == 1) {
    if (fork() == 0) {
      run_master_task(argc, argv);
      return 0;
    }
  }
  return run_tile_server(argc, argv);
}


void set_docroot(int argc, char **argv, char *docroot){
  if(argc>=3){
    if(argv[2][0] != '/')
      sprintf(docroot,"./%s",argv[2]);
    else
      strcpy(docroot,argv[2]);
    
    if(docroot[strlen(docroot)-1]=='/')
      docroot[strlen(docroot)-1] = '\0';
  }
  else
    strcpy(docroot,".");
}



void get_request(int fd, char *reqbuf){
  int i;
  server_read(fd,reqbuf,8*STDLEN);
  
  for(i=0;i<STDLEN;i++)
    if(reqbuf[i]=='\r')
      reqbuf[i]='\0';
  return;
}


void get_file(char **fnamepnt, char *reqbuf){
  int i;
  *fnamepnt = reqbuf;
  for(i=0; ;i++){
    if(reqbuf[i]=='/'){
      (*fnamepnt) += i;
      break;
    }
  }
  for(; ;i++){
    if(reqbuf[i]==' '){
      reqbuf[i] = '\0';
      break;
    }
  }
  return;
}


ssize_t server_read(int fd, char *buf, int len){
  return read(fd,buf,len);
}


ssize_t server_write(int fd, const char *buf, int len){
  ssize_t lensent;
  lensent = write(fd,buf,len);
  if(lensent == len)
    return lensent;
  else if (lensent < 0) return len; // ignore errors
  else{
    printf("all data not sent.\n"); fflush(stdout);
    return (lensent + server_write(fd,buf+lensent,len-lensent));
  }
}


void send_headers(int fd, char *filename, char *buf){
  struct stat info;
  if(stat(filename, &info) < 0)
    return;
  sprintf(buf,"HTTP/1.0 200 OK\r\n");
  sprintf(buf,"%sServer: Picoweb Server\r\n", buf);
  sprintf(buf,"%sAccess-Control-Allow-Origin: *\r\n", buf);
  sprintf(buf,"%sContent-length: %d\r\n", buf, (int) info.st_size);
  sprintf(buf,"%sContent-type: %s\r\n\r\n", buf, get_ftype(filename));
  server_write(fd,buf,strlen(buf));
  /*printf(buf);*/
}

void send_json_headers(int fd, char *buf, int size){
  sprintf(buf,"HTTP/1.0 200 OK\r\n");
  sprintf(buf,"%sServer: Picoweb Server\r\n", buf);
  sprintf(buf,"%sAccess-Control-Allow-Origin: *\r\n", buf);
  sprintf(buf,"%sContent-length: %d\r\n", buf, size);
  sprintf(buf,"%sContent-type: application/json\r\n\r\n", buf);
  server_write(fd,buf,strlen(buf));
}

void send_json_http(int fd, char *header_buf, const char *data_buf, int data_size){
  int i;
  send_json_headers(fd, header_buf, data_size);
  for (i = 0; i < data_size; i += BUFLEN) {
    if (i + BUFLEN < data_size) server_write(fd, data_buf + i, BUFLEN);
    else server_write(fd, data_buf + i, data_size - i);
  }
}

const char *get_ftype(char *filename){
  char ext[4];
  strcpy(ext,filename+strlen(filename)-3);
  if(strcmp(ext,"tml")==0)
    return "text/html";
  else if(strcmp(ext,"jpg")==0)
    return "image/jpeg";
  else if(strcmp(ext,"gif")==0)
    return "image/gif";
  else if(strcmp(ext,"mp3")==0)
    return "audio/mpeg";
  else if(strcmp(ext,"css")==0)
    return "text/css";
  else if(strcmp(ext,"son")==0)
    return "application/json";
  else
    return "text/plain";
}


void send_404(int fd){
  char msg[2*STDLEN];
  sprintf(msg,"HTTP/1.0 404 Not Found\r\nContent-Type: text/html\r\n\r\n");
  sprintf(msg,"%s<html>\n<body style='font-size:48'>\n<hr />\n404 File Not Found\n<hr />\n</body>\n</html>",msg);
  server_write(fd,msg,strlen(msg));
}


int open_listenfd(int port){
  int listenfd, optval=1;
  struct sockaddr_in serveraddr;

  if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    return -1;
  if(setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int)) < 0)
    return -1;
  
  bzero((char *) &serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  serveraddr.sin_port = htons((unsigned short) port);
  
  if(bind(listenfd,(SA *)&serveraddr, sizeof(serveraddr)) < 0)
    return -1;
  if(listen(listenfd, LISTENQ) < 0)
    return -1;

  return listenfd;
}


