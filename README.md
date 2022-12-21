# asio
Asio C++ Library

   为什么要fork asio作为主要开发分支？主要是为在开发的同时 又能及时的跟踪原作者最新的修改和bug修改，以及新的特性的更新，导致后期项目或者扩展更加容易升级。
   现在新的服务器架构sdk以asio为基础库，上层封装设计，底层全部采用asio 设计新的通用的服务器架构。



#### 来自fork asio 官方源码
- https://github.com/chriskohlhoff/asio.git

#### 上游仓库和本地仓库的合并和跟踪操作步骤

1.添加上游仓库

```
git remote add upstream https://github.com/chriskohlhoff/asio.git
```
2.拉去上游变动更新
```
git fetch upstream
```
3.合并本地和上游更新
```
git merge upstream/master 或者 git rebase upstream/master
```
4.更新合并后的推送到远程主分支中
```
git push origin master
```
#### 新的特性
