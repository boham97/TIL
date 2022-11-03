<template>
  <div>
    <h1>점심 메뉴</h1>
    <button>pick one</button>
    <p id="menu">000</p>
  </div>
</template>

<script>
export default {
    name: 'TheLunch',
    data() {
        //const menus = ['짜장', '짬뽕', '탕수육']
        const menu = document.querySelector('#menu')
        menu.innerText = '짜장'
    }
}
//const menus = ['짜장', '짬뽕', '탕수육']
</script>

<style>

</style>