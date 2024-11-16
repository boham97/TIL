```



ModelMapper modelMapper = new ModelMapper();
        modelMapper.getConfiguration().setMatchingStrategy(MatchingStrategies.STRICT);
// 완전히 일치하면 매핑
        UserEntity userEntity = modelMapper.map(userDto, UserEntity.class);
```

#### dependency

```java
implementation group: 'org.modelmapper', name: 'modelmapper', version: '2.3.8'`
```



### modelmapper

`ModelMapper` 란 서로 다른 object 간의 필드 값을 자동으로 mapping 해주는 library

builder느낌
