# GameServer Tutorial

C++20 및 Linux `epoll` 기반의 비동기 게임 서버 네트워크 프로젝트입니다.

## 디렉토리 구조

- `networking/`: Socket, Acceptor, Session, Epoll 등 네트워크 인프라
- `biz/`: 비즈니스 로직 및 프로토콜
- `main.cpp`: 서버 실행 파일

## 빌드 및 실행

```bash
make
./bin/gameserver
```
