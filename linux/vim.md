VIM으로 수정시 임시 파일(.swp) 생성후 해당 파일에서 수정 진행
저장시 원래파일과 링크 제거후 임시파일과 링크 
즉 inode 바뀜

1. 작은 파일 (기본값)

새로운 임시 파일을 생성하고 원본 파일명으로 rename
이 경우 아이노드가 바뀝니다

2. 큰 파일 또는 특정 조건

원본 파일을 직접 덮어쓰기 (in-place editing)
이 경우 아이노드가 유지됩니다

vim이 in-place 편집을 사용하는 경우:

파일이 매우 클 때
하드링크가 여러 개 있을 때
특별한 파일 권한이나 속성이 있을 때
writebackup 옵션이 꺼져있을 때