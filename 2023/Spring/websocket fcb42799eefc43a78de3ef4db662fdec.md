# websocket

생성일: 2023년 4월 18일 오전 11:00
작성자: 보성 강

# web socket

- HTML5 표준안의 일부로 WebSocket API(이후 WebSocket)가 등장했다.
- WebSocket이 기존의 TCP Socket과 다른 점은 최초 접속이 일반 HTTP Request를 통해 HandShaking 과정을 통해 이뤄진다
- HTTP Request를 그대로 사용하기 때문에 기존의 80, 443 포트로 접속을 하므로 추가 방화벽을 열지 않고도 양방향 통신이 가능하고, HTTP 규격인 CORS 적용이나 인증 등 과정을 기존과 동일하게 가져갈 수 있다.

## 예제 1

[http://it-archives.com/222147127689/](http://it-archives.com/222147127689/)

- jsp로 테스트하므로 application property에 mvc추가
- jsp 경로는 src/main/webapp/WEB-INF/views/chat.jsp

```java
spring.mvc.view.prefix = /WEB-INF/views/
spring.mvc.view.suffix=.jsp
```

dependency 추가

```java
dependencies {
    implementation 'org.springframework.boot:spring-boot-starter-websocket'
    compileOnly( group: 'org.apache.tomcat.embed', name: 'tomcat-embed-jasper', version: '9.0.1')
    implementation 'org.apache.tomcat.embed:tomcat-embed-jasper'
    implementation 'javax.servlet:jstl'
}
```

- websocket에서 제공하는 annotation있다.

| 어노테이션      | 설 명                           |
| ---------- | ----------------------------- |
| @OnOpen    | 클라이어트가 접속할 때 발생하는 이벤트         |
| @OnClose   | 클라이언트가 브라우저를 끄거나 다른 경로로 이동할 때 |
| @OnMessage | 메시지가 수신되었을 때                  |

- 처리해야하는부분
  
  - 웹소켓 연결 성립하는 경우
  - 웹소켓 메시지를 수신할 때
  - 연결 해제 하는 경우
  - 웹소켓 에러떄 호출할 함수
  - 웹소켓 연결된 모든 사용자에게 메세지 전송

- 참고 사항
  
  - cors 적용하려면  config를 작성해야하는데 해당 예제에는 없따
  
  - session을 제거하는 로직은 없는것 같다.
    
    ## 예제2
    
    [https://dev-gorany.tistory.com/3](https://dev-gorany.tistory.com/3)
  
  - 다수와 채팅 예제
  
  - TextWebSocketHandler를 extend받아 구현할 수 있다
  
  - <async-supported> 태그 → 클라이언트가 다수인 경우 비동기처리를하기위해 ? 메세지 큐없이도 비동기 처러가 되는거 같다
    
    ## 예제3
    
    [https://dev-gorany.tistory.com/212](https://dev-gorany.tistory.com/212)
    
    ```java
    package com.chat3;
    
    import lombok.RequiredArgsConstructor;
    import org.springframework.context.annotation.Configuration;
    import org.springframework.web.socket.config.annotation.EnableWebSocket;
    import org.springframework.web.socket.config.annotation.WebSocketConfigurer;
    import org.springframework.web.socket.config.annotation.WebSocketHandlerRegistry;
    
    @Configuration
    @RequiredArgsConstructor
    @EnableWebSocket
    public class WebSocketConfig implements WebSocketConfigurer {
    
      private final ChatHandler chatHandler;
    
      @Override
      public void registerWebSocketHandlers(WebSocketHandlerRegistry registry) {
    
          registry.addHandler(chatHandler, "ws/chat").setAllowedOrigins("*");
      }
    }
    ```
  
  - config에서 연결할 주소와 cors처리
    
    [https://not-to-be-reset.tistory.com/88](https://not-to-be-reset.tistory.com/88)
  
  - json으로 보낼때 예시
    
    ![Untitled](websocket%20fcb42799eefc43a78de3ef4db662fdec/Untitled.png)

WebSocketSession 엔 Map 이 있어 원하는 데이터를 추가 할 ㅅ수 있따

```java
public void afterConnectionEstablished(WebSocketSession session) throws Exception {
        session.getAttributes().put("pk","1234");
        System.out.println(session.getAttributes().values());
        list.add(session);

        log.info(session + " 클라이언트 접속");
    }
```