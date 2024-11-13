# ArrayRotate

| [中文版](./README.md) | [English Version](./README.en.md) |

## 介绍

对于 C++ 标准库的 std::rotate(first, mid, last) 函数的深度研究和优化

## 起因

某日，我在 `知乎` 偶然刷到一个帖子：[数组循环左移 是否存在更快的方法？](https://www.zhihu.com/question/499819224)。

很不起眼对吧，确实，我看了一些人的解答以后，才知道 C++ 标准库还有 std::rotate() 这个函数，我估计你可能也没听说过吧。它的作用是旋转 STL 容器中的元素，这种需求一般很少，所以一般人都不太知道。其实，我个人觉得它有点类似于 memmove() 这类函数的思想。如果说 std::rotate() 函数用得不多，但是 memmove() 函数还是用应用场景的。memmove() 的技巧来自于，如果目标区域和源区域有重叠的话，应该怎么处理。当然，std::rotate() 如果作用于非 POD (Plain old data structure) 类型，那跟 memmove() 还是有区别的，但是思想是可以借鉴的，主要的问题点是 Cache Friendly，对于这种拷贝、复制、交换类似的操作，缓存不友好，有时候性能会差很多的。

## 优化的思路

11111111111

## GitHub stats

[![shines77's GitHub stats](https://github-readme-stats.vercel.app/api?username=shines77&show_icons=true&theme=radical)](https://github.com/anuraghazra/github-readme-stats)

## Top Languages

[![shines77's Top Langs](https://github-readme-stats.vercel.app/api/top-langs/?username=shines77&theme=radical)](https://github.com/anuraghazra/github-readme-stats)
