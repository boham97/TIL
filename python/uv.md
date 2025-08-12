uv sync
Python 패키지 매니저인 uv의 명령어로, 프로젝트의 의존성을 동기화

uv sync --no-dev 개발 의존성 제외
특정 그룹만 설치:
bashuv sync --only-group test
uv sync --only-group docs

Python 버전 지정:
bashuv sync --python 3.11

의존성 변경시 uv sync 다시

uv run main.py로 실행 가능