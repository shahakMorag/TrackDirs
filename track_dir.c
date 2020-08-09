#define _XOPEN_SOURCE (600)

#include <ftw.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/inotify.h>

#define MAX_NAME_LENGTH (100)
#define BUFFER_LENGTH ((sizeof(struct inotify_event) + MAX_NAME_LENGTH) * 10)

char inotify_buffer[BUFFER_LENGTH];
bool should_recursively_notify = true;
int inotify_fd = -1;

void display_event(struct inotify_event * event) {
	printf(" wd =%2d; ", event->wd);

	if (event->cookie > 0) printf("cookie =%4d; ", event->cookie);

	printf("mask = ");

	if (event->mask & IN_ACCESS) printf("IN_ACCESS ");
	if (event->mask & IN_ATTRIB) printf("IN_ATTRIB ");
	if (event->mask & IN_CLOSE_NOWRITE) printf("IN_CLOSE_NOWRITE ");
	if (event->mask & IN_CLOSE_WRITE) printf("IN_CLOSE_WRITE ");
	if (event->mask & IN_CREATE) printf("IN_CREATE ");
	if (event->mask & IN_DELETE) printf("IN_DELETE ");
	if (event->mask & IN_DELETE_SELF) printf("IN_DELETE_SELF ");
	if (event->mask & IN_IGNORED) printf("IN_IGNORED ");
	if (event->mask & IN_ISDIR) printf("IN_ISDIR ");
	if (event->mask & IN_MODIFY) printf("IN_MODIFY ");
	if (event->mask & IN_MOVE_SELF) printf("IN_MOVE_SELF ");
	if (event->mask & IN_MOVED_FROM) printf("IN_MOVED_FROM ");
	if (event->mask & IN_MOVED_TO) printf("IN_MOVED_TO ");
	if (event->mask & IN_OPEN) printf("IN_OPEN ");
	if (event->mask & IN_Q_OVERFLOW) printf("IN_Q_OVERFLOW ");
	if (event->mask & IN_UNMOUNT) printf("IN_UNMOUNT ");

	printf("\n");

	if (event->len > 0) printf(" name = %s\n", event->name);
}

int add_watch(const char *fpath, const struct stat *sb,
		                          int typeflag, struct FTW *ftwbuf) {
	if (FTW_D == typeflag) {
		if (-1 == inotify_add_watch(inotify_fd, fpath, IN_ALL_EVENTS)) {
			printf("failed watching %s\n", fpath);
			perror("inotify_add_watch");
			return -1;
		}
	}
	return 0;
}

int recursively_notify(const char * path, int inotify_fd) {
	if (-1 == nftw(path, add_watch, 10, FTW_MOUNT | FTW_PHYS)) {
		perror("nftw");
		return -1;
	}
	return 0;
}	


int main(int argc, char * argv[]) {
	int status = -1;

	if (argc < 2) {
		printf("usage: <path> ...\n");	
		goto exit;
	}


	inotify_fd = inotify_init();
	if (-1 == inotify_fd) {
		perror("inotify_init");
		goto exit;
	}

	for (int i = 1; i < argc; ++i) {
		char * file_name = argv[i];
		if (should_recursively_notify) {
			status = recursively_notify(file_name, inotify_fd);
		}
		else {
			status = inotify_add_watch(inotify_fd, file_name, IN_ALL_EVENTS);
		}

		if (-1 ==  status) {
			printf("failed watching %s\n", file_name);
			perror("inotify_add_watch");
			goto close_inotify_fd;	
		}

	}

	for (;;) {
		int num_read = read(inotify_fd, inotify_buffer, BUFFER_LENGTH);
		switch(num_read)
		{
			case 0:
			case -1:
				perror("read");
				goto close_inotify_fd;
			default:
				for (char * p = inotify_buffer; p < inotify_buffer + num_read;) {
					struct inotify_event * event = (struct inotify_event *)p;			
					display_event(event);
					p += sizeof(struct inotify_event) + event->len;
				}
		}
	}


close_inotify_fd:	
	close(inotify_fd);
exit:
	return status;

}
