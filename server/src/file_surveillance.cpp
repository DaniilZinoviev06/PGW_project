#include <helpers/file_surveillance.h>
#include <sys/inotify.h>
#include <unistd.h>

// сначала использовал только mofify event, но изменяя через GNOME редактор только при первом срабатывании сигнал шел, что файл изменен
// в итоге прочитал, что эти редакторы пересоздают файл заново, поэтому логику пришлось усложнить
// конечно можно было бы еще интереснее реализацию сделать, но все рабоатет, программа считывает изменения файла во время работы

// взял из Керриска / динамический расчет размера буффера в макросе, учитываем структуру
// NAME_MAX - макс. имя файла в системе
// размер структуры понятно
// и умножить на 10, вообще, как я понял, позволяет за один вызов read обработать до 10 событий
#define BUFFER_SIZE (10 * (sizeof(struct inotify_event) + NAME_MAX + 1))

// Здесь реализация отслеживания файла, делал по книге Майкла Керриска "Linux API"

void FileSurveillance::monitoring() {
    // дескриптор inotify
    conf_fd = inotify_init();
    if (conf_fd == -1) {
        std::cerr << "error inotify" << std::endl;
        return;
    }

    //inotify_add_watch для указания цели слежки / принимает дескриптор, путь к файлу, событие(в моем случае - изменение)
    wd = inotify_add_watch(conf_fd, file_path.c_str(), IN_MODIFY | IN_MOVE_SELF | IN_DELETE_SELF);
    if (wd == -1) {
        std::cerr << "error inotify" << std::endl;
        return;
    }

    // лямбда для потока
    stalker = std::thread([this] {
        char buffer[BUFFER_SIZE];

        // бесконечный цикл со считыванием событий из файлового дескриптора
        while (on) {
            ssize_t numRead = read(conf_fd, buffer, BUFFER_SIZE);
            if (numRead == 0) { std::cerr << "read notify fd" << std::endl; break; };
            // if (numRead == -1) std::cerr << "error read from buffer" << std::endl;

            if (numRead == -1) {
                if (errno == EAGAIN || errno == EINTR) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    continue;
                }
            }

            for (char* p = buffer; p < buffer + numRead; ) {
                struct inotify_event* event = (struct inotify_event *) p;

                if (event->mask & (IN_MODIFY | IN_MOVE_SELF | IN_DELETE_SELF)) {
                    if (event->mask & (IN_MOVE_SELF | IN_DELETE_SELF)) {
                        inotify_rm_watch(conf_fd, wd);
                        wd = inotify_add_watch(conf_fd, file_path.c_str(), IN_MODIFY | IN_MOVE_SELF | IN_DELETE_SELF);
                        if (wd == -1) {
                            std::cerr << "watch failed: " << strerror(errno) << std::endl;
                            break;
                        }
                    }
                    callback();
                }
                p += sizeof(struct inotify_event) + event->len;
            }
        }
    });
}

void FileSurveillance::stop_monitoring() {
    on = false;

    if (wd != -1) {
        inotify_rm_watch(conf_fd, wd);
        wd = -1;
    }
    if (conf_fd != -1) {
        close(conf_fd);
        conf_fd = -1;
    }
    if (stalker.joinable()) {
        stalker.join();
    }
}



