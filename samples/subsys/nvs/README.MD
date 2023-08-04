### 说明

因cmakelists.txt中的
~~~
board_runner_args(pyocd "--erase")
~~~
被使能时不能正常烧录，因此从zephyr的同级目录进行拷贝，其他代码无改动。

