# ArrayRotate

| [中文版](./README.md) | [English Version](./README.en.md) |

## Introduction

Deeply research and optimization about C++ standard library std::rotate(first, mid, last) function.

## Cause

One day, I accidentally came across a post on Zhihu.com: [Is there a faster way to rotate left in an array?](https://www.zhihu.com/question/499819224).

It's quite unremarkable, isn't it? Indeed, after reading some people's answers, I found out that the C++ standard library also has the std::rotate() function. I guess you may not have heard of it either. Its function is to rotate elements in STL containers, which is generally rare and not well known to most people. Actually, I personally think it's a bit similar to the idea of functions like memmove(). If the std::rotate() function is not used much, but the memmove() function is still used in application scenarios. The technique of memmove() comes from how to handle overlapping target and source regions. Of course, if std::rotate() is applied to non POD (Plain old data structure) types, it is still different from memmove(), but the idea can be borrowed. The main problem is Cache Friendly, which is not cache friendly for operations such as copy, duplicate, and swap, and sometimes the performance may be much worse.

## Optimization approach

11111111111

## GitHub stats

[![shines77's GitHub stats](https://github-readme-stats.vercel.app/api?username=shines77&show_icons=true&theme=radical)](https://github.com/anuraghazra/github-readme-stats)

## Top Languages

[![shines77's Top Langs](https://github-readme-stats.vercel.app/api/top-langs/?username=shines77&theme=radical)](https://github.com/anuraghazra/github-readme-stats)
