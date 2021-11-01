//所有带双斜杠符号（C 语言注释符）的文本都应该要删除

## Checklist

* [ ] 已修改对应的 issue的状态
* [ ] MR 的Milestones、Lable都已正确设置
* [ ] MR 的 Title、Commit message 足够的简洁且精准
* [ ] 足够的信息帮助 reviewer 理解 feature 的大致实现或者 bug 的相关细节
* [ ] Ci pipeline 已运行且通过
* [ ] 等待 merge

## Related

//如果没有相关的链接或者文档，可删除此章节(建议在此关联上整理的技术文档链接)

* 相关的子 MR
* 如果这个 MR 依赖别的的另一个仓库的新commit，这里填写仓库和commit id
* 如果这是一个 backport MR，这里填写原始 MR

## Release notes (Mandatory)

//每一个 MR 都应该强制编写 Release notes，特别是 feature 类 MR 在 commit message 中无法体现出更多的信息。一些无关紧要的注释拼写 fix 等可自行斟酌是否需要此章节

* [Ai/noise cancellation] Changed/fixed/updated CSK4002NC lib/respak
* [System/os] Added support of cskxxx SMP in FreeRTOS
* [Peripheral/driver] Fixed xxx (//相关 issue 的 link)

## Breaking change notes

//如果没有 breaking change 可删除本节

* [driver/uart] `uart_transfer_data()` 函数增加多一个参数，用来支持 polling 传输与 DMA 传输
