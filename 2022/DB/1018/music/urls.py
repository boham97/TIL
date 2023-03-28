from django.urls import path
from . import views

app_name = 'articles'
urlpatterns = [
    path('artists/', views.artist_cr),
    path('artists/<int:artist_pk>/', views.artist_rud),
    path('music/<int:music_pk>/', views.music_detail),
    path('music/', views.music_all),
    path('artists/<int:artist_pk>/music/', views.music_create),

]