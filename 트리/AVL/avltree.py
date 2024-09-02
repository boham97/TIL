class Node():
    def __init__(self, key:int, height:int, left= None, right =None):
        self.key = key
        self.height = height
        self.left = left
        self.right = right


class Avl:
    def __init__(self):
        self.root = None

    def height(self, node:Node) -> int:
        return 0 if node == None else node.height

    def put(self, key:int) -> None:
        self.root = self.put_item(self.root, key)

    def put_item(self, node:Node, key:int) -> Node:
        if node == None:
            return Node(key, 1)
        if node.key > key:
            node.left = self.put_item(node.left, key)
        elif node.key < key:
            node.right = self.put_item(node.right, key)
        else:
            print("you cant put same key at AVL!")
        node.height = max(self.height(node.left), self.height(node.right)) + 1
        return self.balance(node) #재귀 타는 거 마다 재 정렬


    def balance(self, node:Node) -> Node:
        if self.different(node) > 1:
            if self.different(node.left) < 0:   #왜 0인데? -> left 길이 늘려서 단순화?
                node.left = self.left_turn(node.left) #한번에 줄읻기?
            node = self.right_turn(node)
        elif self.different(node) < -1:
            if self.different(node.right) > 0:
                node.right = self.right_turn(node.right)
            node = self.left_turn(node)
        return node
    
    def different(self, node:Node) -> int:
        return self.height(node.left) - self.height(node.right)

    def left_turn(self, node:Node) -> Node:
        x = node.right
        node.right = x.left
        x.left = node
        node.height = max(self.height(node.right), self.height(node.left)) + 1
        x.height = max(self.height(x.right), self.height(x.left)) + 1
        return x

    def right_turn(self, node:Node) -> Node:
        x = node.left
        node.left = x.right
        x.right = node
        node.height = max(self.height(node.right), self.height(node.left)) + 1
        x.height = max(self.height(x.right), self.height(x.left)) + 1
        return x