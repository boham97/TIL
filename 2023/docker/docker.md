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
