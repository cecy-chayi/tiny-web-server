#include <unistd.h>
#include "server/webserver.h"
#include <sys/param.h>
#include <sys/stat.h>
#include <time.h>
#include <cstring>

int init_daemon(void) 
{ 
	int pid; 
	int i; 
	/*忽略终端I/O信号，STOP信号*/
	signal(SIGTTOU,SIG_IGN);
	signal(SIGTTIN,SIG_IGN);
	signal(SIGTSTP,SIG_IGN);
	signal(SIGHUP,SIG_IGN);
	
	pid = fork();
	if(pid > 0) { exit(0); /*结束父进程，使得子进程成为后台进程*/ }
	else if(pid < 0) {  return -1; }
	std::cout << " 后台进程" << std::endl;
	/*建立一个新的进程组,在这个新的进程组中,子进程成为这个进程组的首进程,以使该进程脱离所有终端*/
	setsid();
	/*再次新建一个子进程，退出父进程，保证该进程不是进程组长，同时让该进程无法再打开一个新的终端*/
	pid=fork();
	if( pid > 0) { std::cout << "daemon: " << pid << std::endl; exit(0); }
	else if( pid< 0) { return -1; }
	std::cout << "非进程组组长进程" << std::endl;
	/*关闭所有从父进程继承的不再需要的文件描述符*/
	for(i=0;i< NOFILE;close(i++));
	/*改变工作目录，使得进程不与任何文件系统联系*/
	if(chdir("/") < 0) {
		return -1;
	}
	/*将文件当时创建屏蔽字设置为0*/
	umask(0);
	/*忽略SIGCHLD信号*/
	signal(SIGCHLD,SIG_IGN); 
	
	return 0;
}


int main(int argc, char* argv[]) {
	int port = 5423;
	if(argc > 3) {
		std::cerr << "webserver argument error" << std::endl;
	}
	for(int i = 1; i < argc; i += 2) {
		if(strcmp(argv[i], "-p") == 0) {
			sscanf(argv[i + 1], "%d", &port);
		}
	}
    // if(init_daemon() < 0) {
	// 	std::cout << "init_daemon error" << std::endl;
	// }
	std::cout << "server run at port[" << port << "]" << std::endl;
    WebServer server(
        port, 3, 60000, false,
        3307, "root", "root", "webserver",
        12, 8, true, 1, 1024
    );
    server.start();
    exit(0);
}