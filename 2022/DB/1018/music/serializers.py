from .models import Artist, Music
from rest_framework import serializers

class MusicListSerializer(serializers.ModelSerializer):
    
    class Meta:
        model = Music
        fields = ('id','title',)


class MusicSerializer(serializers.ModelSerializer):
    
    class Meta:
        model = Music
        fields = '__all__'
        read_only_fields= ('artist',)


class ArtistListSerializer(serializers.ModelSerializer):

    class Meta:
        model = Artist
        fields = '__all__'


class ArtistSerializer(serializers.ModelSerializer):
    music_set = serializers.PrimaryKeyRelatedField(many=True, read_only=True)
    music_count = serializers.IntegerField(source='music_set.count', read_only=True)

    class Meta:
        model = Artist
        fields = '__all__'