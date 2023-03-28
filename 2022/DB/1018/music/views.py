from .models import Music, Artist
from django.shortcuts import render
from rest_framework.decorators import api_view 
from .serializers import MusicSerializer, MusicListSerializer, ArtistListSerializer, ArtistSerializer
from rest_framework.response import Response
from rest_framework import status
from django.shortcuts import get_list_or_404, get_object_or_404
# Create your views here.
from music import serializers
@api_view(['GET','POST'])
def artist_cr(request):
    if request.method == 'GET':
        artists = get_list_or_404(Artist)
        serializer = ArtistListSerializer(artists, many=True)
        return Response(serializer.data)
    elif request.method == 'POST':
        serializer = ArtistSerializer(data=request.data)
        if serializer.is_valid(raise_exception=True):
            serializer.save()
            return Response(serializer.data, status=status.HTTP_201_CREATED)


@api_view(['POST'])
def music_create(request, artist_pk):
    artist = Artist.objects.get(pk=artist_pk)
    serializer = MusicSerializer(data=request.data)
    if serializer.is_valid(raise_exception=True):
        serializer.save(artist=artist)
        return Response(serializer.data, status=status.HTTP_201_CREATED)


@api_view(['GET'])
def music_all(request):
    musices = get_list_or_404(Music)
    serializer = MusicListSerializer(musices, many=True)
    return Response(serializer.data)


@api_view(['GET'])
def artist_rud(request, artist_pk):
    artist = get_object_or_404(Artist, pk=artist_pk)
    serializer = ArtistSerializer(artist)
    return Response(serializer.data)


@api_view(['GET','PUT','DELETE'])
def music_detail(request, music_pk):
    music = get_object_or_404(Music, pk=music_pk)
    if request.method == 'GET':
        serializer = MusicSerializer(music)
        return Response(serializer.data)
    elif request.method == 'PUT':
        serializer = MusicSerializer(music,data=request.data)
        if serializer.is_valid(raise_exception=True):
            serializer.save()
            return Response(serializer.data)
    elif request.method == 'DELETE':
        music.delete()
        return Response({ 'id': music_pk }, status=status.HTTP_204_NO_CONTENT)
