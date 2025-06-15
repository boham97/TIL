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
    stat()담기는 정보들
    struct stat {
        dev_t     st_dev;     // 파일이 존재하는 디바이스 ID
        ino_t     st_ino;     // inode 번호 (파일 고유 ID)
        mode_t    st_mode;    // 파일 타입과 퍼미션 (예: 0755)
        nlink_t   st_nlink;   // 하드 링크 수
        uid_t     st_uid;     // 파일 소유자 UID
        gid_t     st_gid;     // 파일 소유자 GID
        off_t     st_size;    // 파일 크기 (바이트)
        time_t    st_atime;   // 마지막 접근 시간
        time_t    st_mtime;   // 마지막 수정 시간
        time_t    st_ctime;   // 마지막 inode 변경 시간
        ...
    };

    IN_MODIFY는 수정중도 감지하므로 아직 끝나지 않은 경우에도 이벤트 감지
    잘못된 결과가 생길수 있음 --> IN_CLOSE_WRITE 이 flag를 쓰자
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/inotify.h>
#include <fnmatch.h>  // 패턴 매칭 함수 사용

#define EVENT_SIZE (sizeof(struct inotify_event))
#define BUF_LEN (1024 * (EVENT_SIZE + 16))


void print_file_info(const char *watch_dir, const char *filename);

int main() {
    const char *watch_dir = "./watched_dir";
    const char *pattern = "error-*.log";

    int fd = inotify_init1(IN_NONBLOCK);                                                        //파일 디스크립터를 논블로킹 모드로 생성
    if (fd < 0) {
        perror("inotify_init1");
        exit(EXIT_FAILURE);
    }
    //inotify_add_watch(fd, full_path_to_file, IN_MODIFY);  //변화 감지
    int wd = inotify_add_watch(fd, watch_dir, IN_CREATE  | IN_CLOSE_WRITE);
    if (wd == -1) {
        perror("inotify_add_watch");
        exit(EXIT_FAILURE);
    }

    printf("Watching for files matching pattern '%s' in %s...\n", pattern, watch_dir);

    //char buf[BUF_LEN] __attribute__((aligned(8)));                                            //공부 필요
    char buf[BUF_LEN] = {0,};                                                               

    while (1) 
    {
        int len = read(fd, buf, BUF_LEN);
        if (len <= 0) 
        {
            usleep(200000);  // 100ms
            continue;
        }

        for (char *ptr = buf; ptr < buf + len;) 
        {   
            struct inotify_event *event = (struct inotify_event *) ptr;                         //inotify_event는 가변 길이 구조체

            if ((event->mask & IN_CREATE) && event->len > 0) 
            {
                // 파일 이름 패턴 매칭
                if (fnmatch(pattern, event->name, 0) == 0) 
                {
                    printf("[ALERT] New matching file created: %s\n", event->name);
                    print_file_info(watch_dir, event->name);
                    // 여기에 알림, 로깅, 추가 처리 코드 넣으면 됨
                }
            }
            else if ((event->mask & IN_CLOSE_WRITE) && event->len > 0)                         //파일 변화 감지
            {
                if (fnmatch(pattern, event->name, 0) == 0) 
                {
                    printf("[ALERT] matching file modify: %s\n", event->name);
                    print_file_info(watch_dir, event->name);
                }
            }
            ptr += EVENT_SIZE + event->len;                                                 //name[]만큼 offset
        }
    }

    close(fd);
    return 0;
}


void print_file_info(const char *watch_dir, const char *filename) 
{
    char full_path[1024];
    snprintf(full_path, sizeof(full_path), "%s/%s", watch_dir, filename);

    struct stat st;
    if (stat(full_path, &st) == -1) {
        perror("stat");
        return;
    }

    printf("파일: %s\n", full_path);
    printf("크기: %ld bytes\n", st.st_size);
    printf("inode 번호: %ld\n", st.st_ino);
}