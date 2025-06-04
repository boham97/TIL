/*
    새로 생긴 로그파일 감지

    최대 감지 제한 있음  `(/proc/sys/fs/inotify/max_user_watches`

    struct inotify_event 
    {
        int      wd;
        uint32_t mask;
        uint32_t cookie;
        uint32_t len;     // name 문자열의 길이
        char     name[];  // 파일 이름이 여기에 붙어 있음 (len 길이만큼)
    };

*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/inotify.h>
#include <fnmatch.h>  // 패턴 매칭 함수 사용

#define EVENT_SIZE (sizeof(struct inotify_event))
#define BUF_LEN (1024 * (EVENT_SIZE + 16))

int main() {
    const char *watch_dir = "./watched_dir";
    const char *pattern = "error-*.log";

    int fd = inotify_init1(IN_NONBLOCK);                                                        //파일 디스크립터를 논블로킹 모드로 생성
    if (fd < 0) {
        perror("inotify_init1");
        exit(EXIT_FAILURE);
    }
    //inotify_add_watch(fd, full_path_to_file, IN_MODIFY);  //변화 감지
    int wd = inotify_add_watch(fd, watch_dir, IN_CREATE);
    if (wd == -1) {
        perror("inotify_add_watch");
        exit(EXIT_FAILURE);
    }

    printf("Watching for files matching pattern '%s' in %s...\n", pattern, watch_dir);

    //char buf[BUF_LEN] __attribute__((aligned(8)));                                            //공부 필요
    char buf[BUF_LEN] = {0,};                                                               

    while (1) {
        int len = read(fd, buf, BUF_LEN);
        if (len <= 0) {
            usleep(100000);  // 100ms
            continue;
        }

        for (char *ptr = buf; ptr < buf + len;) {   
            struct inotify_event *event = (struct inotify_event *) ptr;                         //inotify_event는 가변 길이 구조체

            if ((event->mask & IN_CREATE) && event->len > 0) 
            {
                // 파일 이름 패턴 매칭
                if (fnmatch(pattern, event->name, 0) == 0) {
                    printf("[ALERT] New matching file created: %s\n", event->name);
                    // 여기에 알림, 로깅, 추가 처리 코드 넣으면 됨
                }
            }
            //else if ((event->mask & IN_MODIFY) && event->len > 0)                         //파일 변화 감지

            ptr += EVENT_SIZE + event->len;                                                 //name[]만큼 offset
        }
    }

    close(fd);
    return 0;
}
