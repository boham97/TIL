from dataclasses import field
from rest_framework import serializers
from .models import Article, Comment

class CommentSerializer(serializers.ModelSerializer):
    class Meta:
        model = Comment
        fields = '__all__'
        read_only_fields = ['article',] #중요!! 사용자 입력이 필요 없으면 모두 여기애


class ArticleListSerializer(serializers.ModelSerializer):
    class Meta:
        model = Article
        fields = ('id', 'title',)


class ArticleSerializer(serializers.ModelSerializer):
    #related manager 이름으로 필드 생성
    comment_set = serializers.PrimaryKeyRelatedField(many=True, read_only=True)
    comment_count = serializers.IntegerField(source='comment_set.count', read_only=True)
    class Meta:
        model = Article
        fields = '__all__'
