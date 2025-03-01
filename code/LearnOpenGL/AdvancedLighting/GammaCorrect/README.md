# 概念

Gamma校正解决的问题是:

对整体亮度(光照强度)的补光.

渲染管线中提供的是指数式的光强变换公式. 现实中我们更倾向于线性的光强变换.

因此, 尽管光强最小值/最大值都是一样的, 但在中间区域, 前者缺失了一些光照强度.

经验值 2.2

```GLSL
 float gamma = 2.2;
 fragColor.rgb = pow(fragColor.rgb, vec3(1.0/gamma));
```

我们对片段颜色的每个分量, 都做了 1.0/2.2 的幂运算. 完成校正.

总体效果: 场景变亮(补光)了.

## sRGB纹理

当我们基于监视器上看到的情况创建一个图像，我们就已经对颜色值进行了gamma校正，所以再次显示在监视器上就没错。由于我们在渲染中又进行了一次gamma校正，图片就实在太亮了。

## 衰减

因为做了补光, 衰减系数也需要调整.

## 个人感受

本来Gamma是为了补光的, 但是后两个操作又是把场景调暗.

所以整体来看, 整个场景的最亮的部分和最暗的部分没有变化, 只是中间过度的光强变得平滑了.