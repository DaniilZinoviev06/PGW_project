<hr><div align="center">
  <a href="#about">О проекте</a>&ensp;&ensp;&ensp;
  <a href="#installation">Установка</a>&ensp;&ensp;&ensp;
  <a href="#settings">Конфигурация</a>&ensp;&ensp;&ensp;
</div><hr>

<h2 id="about">:scroll: О проекте</h2>
<p> Данный проект представляет собой упрощённую модель сетевого компонента PGW (Packet Gateway), способную обрабатывать UDP-запросы, 
  управлять сессиями абонентов по IMSI, вести CDR-журнал, предоставлять HTTP API, поддерживать чёрный список IMSI и корректно завершать работу.</p>

  В качестве основы мною были взяты
  - https://github.com/DaniilZinoviev06/simple_pgw
  - https://github.com/stolex1y/simple-pgw

<h2 id="installation">:scroll: Установка и запуск</h2>

Клонируйте репозиторий:
```bash
git clone https://github.com/DaniilZinoviev06/PGW_project
```

Скомпилируйте при помощи CMAKE:
```
mkdir build
cd build
cmake ../
cmake --build .
```

Запустите:
```
src/simple_pgw - PGW
client/pgw_client - клиент
test/simple_pgw_tests - тесты
```

Пример сценария

Запускаем сервер:
```
./simple_pgw
```

Запускаем клиент:
```
./pgw_client "001010000562401"
curl "http://localhost:49156/check_subscriber?imsi=000134235345453"
```

<h2 id="settings">:scroll: Конфигурация</h2>
<p>Конфигурация осуществляется при помощи 2 JSON файлов</p>
<br>
  
<p>Сервер:</p>

```
{
  "udp_ip": "127.0.0.1",
  "udp_port": 49155,
  "session_timeout_sec": 30,
  "cdr_file": "../../logs/cdr.csv",
  "http_port": 49156,
  "graceful_shutdown_rate": 10,
  "log_file": "pgw.log",
  "log_level": "INFO",
  "blacklist": [
    "001010123456789",
    "001010000000001",
    "001010524060001",
    "001010079786501",
    "001013407635601",
    "001010000562401",
    "001010050050001",
    "001010080070001",
    "001010000060501"
  ]
}
```

Клиент:
```
{
  "server_ip": "127.0.0.1",
  "server_port": 49155,
  "log_file": "../../logs/client.log",
  "log_level": "INFO"
}
```

<hr>
ИНФ.

На данный момент не реализовано:
- Логирование в коде, с уровнями debug, info, warn, critical, error
- Команда /stop, которая завершает работу приложения, используя механизм graceful offload: сессии удаляются с заданной скоростью, происходит запись CDR.
- Большая часть кода не протестирована

Задачи на спринт до 28.07
- сделать то, что перечислено выше
- улучшить структуру проекта, из некоторых h файлов вынести реализацию в cpp
- написать скрипты для автоматизации развертывания и работы с профилеровщиком perf
- добавить inotify для прослушивания лог файла на изменения в процессе работы
