### **Kafka Exporter 주요 플래그**

- **`--kafka.server`**: Kafka 브로커의 주소 (예: `localhost:9092`).
- **`--web.listen-address`**: Kafka Exporter가 메트릭을 제공할 포트. 기본값은 `9181`입니다.
- **`--kafka.zookeeper`**: Kafka의 ZooKeeper 연결 문자열 (ZooKeeper 모드에서 사용할 때).
- **`--kafka.version`**: Kafka 브로커의 버전 (옵션, 기본값은 자동 감지).
- **`--ignore-consumer-groups`**: 소비자 그룹 관련 메트릭을 무시하도록 설정.

### **Kafka Exporter 메트릭**

Kafka Exporter는 다음과 같은 주요 메트릭을 제공합니다:

1. **Kafka 브로커 메트릭**:
   
   - `kafka_server_brokeruptime_seconds`: Kafka 브로커의 업타임 (초 단위).
   - `kafka_server_broker_topic_metrics`: 각 브로커의 토픽 관련 메트릭.

2. **파티션 및 리더 메트릭**:
   
   - `kafka_partition_leader_count`: 각 파티션의 리더 수.
   - `kafka_partition_under_replicated`: 복제되지 않은 파티션의 수.

3. **소비자 메트릭**:
   
   - 1. **`kafka_consumer_fetch_rate`**
        
        - **설명**: 소비자가 Kafka로부터 데이터를 fetch하는 속도(초당 fetch된 메시지 수)를 나타냅니다.
        - **사용 예**: 이 메트릭은 소비자가 얼마나 자주 Kafka 브로커로부터 데이터를 가져오는지 나타냅니다. 이 값이 낮으면 소비자가 데이터를 충분히 가져오지 못하고 있음을 의미합니다.
        - **예시**: `kafka_consumer_fetch_rate{consumer_group="my_group", topic="my_topic", partition="0"}`
     
     2. **`kafka_consumer_fetch_latency_ms`**
        
        - **설명**: 소비자가 Kafka로부터 메시지를 fetch하는 데 걸리는 시간(밀리초)을 나타냅니다.
        - **사용 예**: 이 메트릭은 fetch 요청에 대한 **대기 시간**을 측정합니다. 지연 시간이 너무 길면 소비자가 데이터를 가져오는 데 시간이 많이 걸리므로, 이 값을 줄여야 할 필요가 있습니다.
        - **예시**: `kafka_consumer_fetch_latency_ms{consumer_group="my_group", topic="my_topic", partition="0"}`
     
     3. **`kafka_consumer_records_lag`**
        
        - **설명**: 각 파티션에 대해 소비자가 처리하지 않은 메시지의 수를 나타냅니다. `lag`는 **소비자 그룹**의 **오프셋 지연**과 동일합니다.
        - **사용 예**: 이 값은 특정 파티션의 메시지 처리 상태를 추적합니다. `lag`가 커지면 소비자가 메시지를 처리하는 속도가 느려지고 있음을 나타냅니다.
        - **예시**: `kafka_consumer_records_lag{consumer_group="my_group", topic="my_topic", partition="0"}`
     
     4. **`kafka_consumer_commit_latency_ms`**
        
        - **설명**: 소비자가 오프셋을 **커밋**하는 데 걸리는 시간(밀리초)을 나타냅니다.
        - **사용 예**: 커밋 시간은 소비자가 메시지를 처리한 후 해당 오프셋을 Kafka에 저장하는 데 소요되는 시간입니다. 이 값이 크면 커밋이 느리다는 것을 의미하며, 이는 지연을 발생시킬 수 있습니다.
        - **예시**: `kafka_consumer_commit_latency_ms{consumer_group="my_group", topic="my_topic"}`
     
     5. **`kafka_consumer_partition_count`**
        
        - **설명**: 소비자가 **할당된 파티션**의 수를 나타냅니다. 소비자 그룹이 여러 파티션을 할당받아 데이터를 읽고 있을 때 사용됩니다.
        - **사용 예**: 이 메트릭은 소비자가 얼마나 많은 파티션을 다루고 있는지 알 수 있습니다. 너무 많은 파티션을 다루면 과부하가 걸릴 수 있으므로 적절한 균형이 필요합니다.
        - **예시**: `kafka_consumer_partition_count{consumer_group="my_group"}`
     
     6. **`kafka_consumer_bytes_consumed_rate`**
        
        - **설명**: 소비자가 초당 얼마나 많은 데이터를 소비하는지 나타내는 메트릭입니다.
        - **사용 예**: 이 메트릭은 소비자가 얼마나 많은 바이트를 Kafka에서 읽고 있는지 추적합니다. **네트워크 대역폭**이 부족한 경우, 이 값을 모니터링하여 최적화할 수 있습니다.
        - **예시**: `kafka_consumer_bytes_consumed_rate{consumer_group="my_group", topic="my_topic", partition="0"}`
     
     7. **`kafka_consumer_error_rate`**
        
        - **설명**: 소비자가 메시지를 처리하는 동안 발생한 오류의 비율을 나타냅니다.
        - **사용 예**: 이 메트릭은 Kafka에서 메시지를 소비하면서 발생한 오류의 비율을 측정합니다. 오류가 많으면 네트워크 문제나 다른 시스템 문제로 인해 메시지 소비가 실패할 수 있습니다.
        - **예시**: `kafka_consumer_error_rate{consumer_group="my_group"}`
     
     8. **`kafka_consumer_time_since_last_poll_ms`**
        
        - **설명**: 마지막 **poll** 호출 후 경과한 시간(밀리초)을 나타냅니다.
        - **사용 예**: 이 메트릭은 소비자가 마지막으로 Kafka에서 메시지를 `poll`한 이후 얼마나 시간이 흘렀는지 알려줍니다. 이 값이 커지면 소비자가 Kafka에서 데이터를 더 이상 자주 읽지 않거나 응답이 지연되는 문제를 나타낼 수 있습니다.
        - **예시**: `kafka_consumer_time_since_last_poll_ms{consumer_group="my_group"}`
     
     9. **`kafka_consumer_idle_time_ms`**
        
        - **설명**: 소비자가 **대기** 상태에 있던 시간(밀리초)을 나타냅니다.
        - **사용 예**: 이 메트릭은 소비자가 새로운 메시지를 처리하기 위해 대기한 시간을 추적합니다. 대기 시간이 길면 메시지 흐름에 지연이 발생하고 있을 수 있습니다.
        - **예시**: `kafka_consumer_idle_time_ms{consumer_group="my_group"}`

4. **프로듀서 메트릭**:
   
   - 1. **`kafka_producer_in_flight_requests`**:
        
        - 현재 전송 중인 요청의 수를 나타냅니다. 이 값이 높으면 프로듀서가 동시에 여러 메시지를 전송하려고 하고 있다는 것을 의미합니다.
     
     2. **`kafka_producer_record_send_rate`**:
        
        - 초당 전송되는 메시지 수를 나타냅니다. 이 메트릭은 프로듀서의 성능을 측정하는 중요한 지표입니다. 이 값이 낮으면 메시지 생산 속도가 떨어지고 있다는 의미입니다.
     
     3. **`kafka_producer_request_latency_ms`**:
        
        - 프로듀서가 요청을 보내고 응답을 받을 때까지의 평균 시간(밀리초 단위)을 나타냅니다. 이 값이 높으면, Kafka 브로커와의 통신이 지연되고 있다는 것을 의미합니다.
     
     4. **`kafka_producer_error_rate`**:
        
        - 프로듀서가 전송 도중 발생한 오류의 비율입니다. 이 값이 높으면, 메시지가 브로커로 정상적으로 전송되지 않고 있다는 의미이며, 네트워크 장애나 브로커의 리소스 부족 등의 원인일 수 있습니다.
     
     5. **`kafka_producer_batch_size`**:
        
        - 프로듀서가 한 번에 전송하는 배치의 크기입니다. 배치 크기가 너무 작으면 성능이 떨어지고, 너무 크면 메모리 소모가 많아질 수 있습니다.

5. **리소스 사용률**:
   
   - `kafka_server_network_processor`: 네트워크 처리기 사용량.
   - `kafka_server_disk_usage`: 디스크 사용량.





### **Kafka Consumer Metrics 활용 방법**

- **성능 모니터링**: 실시간으로 `lag`와 `fetch rate`를 추적하여 **소비자 성능**이 어떻게 변하는지 모니터링할 수 있습니다.
- **알림 설정**: 지연 시간이나 오류 비율이 일정 임계값을 초과하면 알림을 설정하여 시스템이 문제를 빠르게 해결하도록 할 수 있습니다.
- **로드 밸런싱**: 너무 많은 파티션을 처리하는 소비자가 있을 경우, 소비자 수를 확장하거나 파티션을 재배치하여 시스템 부하를 분산시킬 수 있습니다.
