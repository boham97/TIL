import sys
from confluent_kafka import Consumer, KafkaException, KafkaError
from concurrent.futures import ThreadPoolExecutor

cnt = 0

# Kafka 소비자 설정
conf = {
    'bootstrap.servers': 'localhost:9092',  # 브로커 주소
    'group.id': 'my_consumer_group',
    'auto.offset.reset': 'earliest',  # 처음부터 메시지를 읽기 시작
    'enable.auto.commit': False,  # 자동 커밋 비활성화
}

# Kafka 소비자 객체 생성
consumer = Consumer(conf)

# 메시지 처리 함수 (각 메시지는 쓰레드에서 처리됨)
#kafka-topics.sh --create --topic my-topic --bootstrap-server localhost:9092 
#kafka-console-producer.sh --broker-list localhost:9092 --topic my-topic
#kafka-topics.sh --list --bootstrap-server localhost:9092
#kafka-topics.sh --describe --topic my-topic --bootstrap-server localhost:9092
#kafka-topics.sh --delete --topic my_topic --bootstrap-server localhost:9092
#kafka-console-producer.sh --broker-list localhost:9092 --topic my-topic
#kafka-topics.sh --describe --topic my-topic --bootstrap-server localhost:9092
# 메시지 소비 함수
def consume_messages():
    # 'my_topic' 토픽을 구독
    consumer.subscribe(['my-topic'])

    # 쓰레드 풀 생성 (5개의 쓰레드로 병렬 처리)
    try:
        while True:
            # 메시지 가져오기
            msgs = consumer.poll(timeout=0.1)
            if msgs is None:
                continue  # 새 메시지가 없으면 계속 대기
            if msgs.error():
                if msgs.error().code() == KafkaError._PARTITION_EOF:
                    print(f'End of partition reached: {msgs.topic()} [{msgs.partition()}] at offset {msgs.offset()}')
                else:
                    cnt += 1
                    print(cnt)
            # 수동으로 오프셋 커밋
            consumer.commit()
    except KeyboardInterrupt:
        print("프로그램 종료")
    finally:
        consumer.close()

# 메시지 소비 시작
if __name__ == '__main__':
    consume_messages()

