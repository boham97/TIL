from .models import Article, Comment
from .forms import ArticleForm, CommentForm
from django.shortcuts import render, redirect, get_object_or_404
from django.contrib.auth.decorators import login_required
from django.views.decorators.http import require_http_methods, require_POST, require_safe

# Create your views here.
@require_safe
def index(request):
    articles = Article.objects.all()
    context = {
        'articles': articles,
    }
    return render(request, 'articles/index.html', context)


@login_required
@require_http_methods(['GET', 'POST'])
def create(request):
    if request.method == 'POST':
        form = ArticleForm(request.POST) 
        if form.is_valid():
            article = form.save(commit=False)
            article.user = request.user
            article = form.save()
            return redirect('articles:detail', article.pk)
    else:
        form = ArticleForm()
    # print(form.errors)
    context = {
        'form': form,
    }
    return render(request, 'articles/create.html', context)


@require_safe
def detail(request, pk):
    article = get_object_or_404(Article, pk=pk)
    comment_form = CommentForm() #댓글 압력 줄
    comments = Comment.objects.filter(article=article)
    context = {
        'article': article,
        'comment_form': comment_form, #html에서 사용하기 위해 c전달받아야
        'comments': comments,
    }
    return render(request, 'articles/detail.html', context)


@require_POST
def delete(request, pk):
    if request.user.is_authenticated:
        article = get_object_or_404(Article, pk=pk)
        article.delete()
    return redirect('articles:index')


@login_required
@require_http_methods(['GET', 'POST'])
def update(request, pk):
    article = get_object_or_404(Article, pk=pk)
    if article.user != request.user:
        return redirect('articles:detail', article.pk)
    if request.method == 'POST':
        # Create a form to edit an existing Article,
        # but use POST data to populate the form.
        form = ArticleForm(request.POST, instance=article)
        if form.is_valid():
            form.save()
            return redirect('articles:detail', article.pk)
    else:
        # Creating a form to change an existing article.
        form = ArticleForm(instance=article)
    context = {
        'article': article,
        'form': form,
    }
    return render(request, 'articles/update.html', context)

@require_POST
def comment_create(request, article_pk):
    if not request.user.is_authenticated:
        return redirect('accounts:login')
    comment_form = CommentForm(request.POST)
    article = Article.objects.get(pk=article_pk)

    if comment_form.is_valid():
        comment = comment_form.save(commit=False) #db에 저장전 데이터 받아오기
        comment.article = article
        comment.save()
        return redirect('articles:detail', article_pk)
    comments = Comment.objects.filter(article=article)
    context = {
        'comment_form': comment_form,
        'article': article,
        'comments':comments,

    }
    return render(request, 'articles/detail.html', context)

@require_POST
def comment_delete(request,comment_pk):
    if not request.user.is_authenticated:
        return redirect('accounts:login')
    comment = Comment.objects.get(pk=comment_pk)
    article = comment.article #삭제전 게시글 정보 저장하여 redirect에 사용함
    comment.delete()

    return redirect('articles:detail', article.pk)