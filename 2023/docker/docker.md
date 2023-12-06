https://docs.docker.com/get-started/what-is-a-container/?uuid=699086B8-FE38-4C94-88F3-920FECE4B993

# container

### an isolated environment for your code

### 1. 도커 시작하기

```
docker run -p 8088:80 -d --name welcome-to-docker docker/welcome-to-docker
```

- docker run: 컨테이너 실행
- -p 8088:80: 로컬 8088에 80포트 사용
- -d: 백그라운드에서 실행
- —-name welcome-to-docker: 컨테이너 이름 설정

https://johngrib.github.io/wiki/why-http-80-https-443/
http는 기본포트 80
https 443

### 2. 도커 데스커탑에서 보기

- db, fe, be 등 컨테이너가 많아질수 있다

### 3. 8088포트로 브라우저ㄲㄲ

 http://localhost:8088

### 4.컨테이너 내 파일들을 볼수 있다

# 도커 실행하기

https://docs.docker.com/get-started/run-your-own-container/

### 1. 깃 클론하기

- 파일이 있어도된다
  
  ### 2. 프로젝트에 도커파일 생성
  
  ```
  cd \path\to\welcome-to-docker
  ```
  
  ```
  type nul > Dockerfile
  ```
  
  ### 3.dockerfile에 instructions 추가
  
  ```
  # syntax=docker/dockerfile:1
  ```

# Start your image with a node base image

FROM node:18-alpine

# Create an application directory

RUN mkdir -p /app

# Set the /app directory as the working directory for any command that follows

WORKDIR /app

# Copy the local app package and package-lock.json file to the container

COPY package*.json ./

# Copy local directories to the working directory of our docker image (/app)

COPY ./src ./src
COPY ./public ./public

# Install node packages, install serve, build the app, and remove dependencies at the end

RUN npm install \
    && npm install -g serve \
    && npm run build \
    && rm -fr node_modules

# Specify that the application in the container listens on port 3000

EXPOSE 3000

# Start the app using serve command

CMD [ "serve", "-s", "build" 

```
### 4. 이미지 빌드
```

docker build -t welcome-to-docker .

```
### 5. 실행
- To run your image as a container, go to the Images tab, and then select Run in the Actions column of your image. When the Optional settings appear, specify the Host port number 8089 and then select Run.
```

`

```null
docker image bulid -t 이미지명[:태그명] Dockerfile경로
```

```null
docker image pull 레파지토리명[:태그명]
```

- `docker start 컨테이너ID`  
  : 컨테이너 시작

- `docker attach 컨테이너ID ` 
  : 컨테이너 접속

- `docker stop 컨테이너ID`  
  : 컨테이너 멈춤

- `docker run 컨테이너ID`  
  : 컨테이너 생성 및 시작

- `docker run -it 컨테이너ID`  
  : 컨테이너 생성 및 시작 및 접속

- `docker rm 컨테이너ID`  
  : 컨테이너 삭제

- `docker exec -it 컨테이너ID /bin/bash`  
  : 실행되고 있던 컨테이너 접속

- `exit`  
  : 컨테이너 빠져나오기

`-e` 환경 변수 설정

`-p` 포트 포워딩

`--network` 네트워크 사용

`--name` 컨테이너 이름 변경

### 네트워크

`docker network ls`

네트워크 목록 확인

`docker network inspect brige`

상세정보 확인

네트워크 설정없이 컨테이너 생성시 docker0 브리지 사용

`disconnect` `connect` 

컨테이너에 network 할당

` docker network create --gateway 172.18.0.1 --subnet 172.18.0.0/16 logicarrier-network`

- 서브넷

<u>네트워크가 작은 조각으로 쪼개져 있는 경우 이러한 조각을 서브넷</u>이라고 부릅니다. 

한 마디로 서브넷은 작은 네트워크라고 할 수 있습니다. 

<u>네트워크 성능 개선을 위해 네트워크 관리지가 효율적으로 자원을 분배하는 것이 바로 서브네팅(subnetting)</u>이라 합니다.

즉, IP 주소를 그룹으로 나누어 관리하는 방법입니다.

### 컴포즈

```
    networks:
      my-network:
        ipv4_address: 172.18.0.100
```

```
networks:
  my-network:
    name: logicarrier-network
    external: true
```

컨테이너에 서브넷 할당

`external: true` 이미 만든 네트워크 사용

`depends on` 해당 컨테이너 run 후 실행

`environment:` 환경변수 변경 로(컬에서 도커환경으로)

네트워크 생성

```
networks:
  my-network:
    name: logicarrier-network
    ipam:
      driver: default
      config:
        - subnet: 172.35.0.0/16
          gateway: 172.35.0.1        
```
