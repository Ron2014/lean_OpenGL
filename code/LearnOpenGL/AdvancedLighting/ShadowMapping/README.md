## 概念

阴影还是比较不好实现的，因为当前实时渲染领域还没找到一种完美的阴影算法。目前有几种近似阴影技术，但它们都有自己的弱点和不足，这点我们必须要考虑到。

视频游戏中较多使用的一种技术是阴影贴图（shadow mapping），效果不错，而且相对容易实现。阴影贴图并不难以理解，性能也不会太低，而且非常容易扩展成更高级的算法（比如 Omnidirectional Shadow Maps和 Cascaded Shadow Maps）。

## 阴影贴图/映射 shadow mapping

回想深度缓冲, 利用相机的投影矩阵, 得到片段的深度值.

如果我们从光源的透视图来渲染场景，并把深度值的结果储存到纹理中会怎样？通过这种方式，我们就能对光源的透视图所见的最近的深度值进行采样。最终，深度值就会显示从光源的透视图下见到的第一个片段了。我们管储存在纹理中的所有这些深度值，叫做深度贴图（depth map）或阴影贴图。

当渲染P片段时, 将其与光源位置建立射线, 利用光源的的投影矩阵, 计算出这个射线的(可见深度), 如果小于两点距离, 就表示在阴影中.

阴影映射由两个步骤组成：

1. 我们渲染深度贴图
2. 我们像往常一样渲染场景，使用生成的深度贴图来计算片段是否在阴影之中

## 深度贴图 Depth Map

初始化

1. 创建2D纹理, 指定分辨率
2. 纹理源格式/目标格式为 GL_DEPTH_COMPONENT
3. 不需要颜色缓冲

渲染深度贴图(阴影灰度图)

1. 取得定向光的观察矩阵view和投影矩阵projection(正交/透视)
2. 使用指定渲染深度贴图的shader, 绘制场景中的物体
3. 顶点着色器中, 将所有顶点的坐标变换到定向光的观察空间
4. 片段着色器留空, 因为没有指定颜色FragColor, 会得到和深度对应的灰度图.

```c
GLfloat near_plane = 1.0f, far_plane = 7.5f;
glm::mat4 lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);
glm::mat4 lightView = glm::lookAt(glm::vec(-2.0f, 4.0f, -1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)); // 方向 2.0, -4.0, 1.0
glm::mat4 lightSpaceMatrix = lightProjection * lightView;
```

正常渲染场景

1. 向所有shader传入定向光的观察矩阵view和投影矩阵projection(正交/透视)
2. 在片段着色器上, 将片段坐标变换至定向光的观察空间, 将深度值与深度贴图的closet深度值作比较.
3. 混合颜色.

### 需要注意的地方

光的观察空间的视锥, 近平面/远平面距离的选择决定了空间的大小.

指定一个固定的深度图分辨率, 填充该图前后进行视口变换viewport, 保证图像的尺寸正确, 方便第二次渲染时采样深度值.

## 阴影失真(Shadow Acne)

阴影贴图受限于分辨率，在距离光源比较远的情况下，多个片段可能从深度贴图的同一个值中去采样。

解决方案

- 阴影偏移（shadow bias）

使用了偏移量后，所有采样点都获得了比表面深度更小的深度值

```c
float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);
float shadow = currentDepth - bias > closestDepth  ? 1.0 : 0.0;
```

## 悬浮(Peter Panning)

使用背面深度不会有错误，因为阴影在物体内部有错误我们也看不见。

解决方案

- 正面剔除（front face culling）


## 过度采样

光照有一个区域，超出该区域就成为了阴影:

1. 正射投影, 这个区域是个长方体
2. 透视投影, 这个区域是个平头截体

**采样的片段, 实际上是被定向光观察空间裁剪的片段.**

- 解决方案

1. 调整近平面/远平面

扩大观察空间的纵深度

2. 让所有超出深度贴图的坐标的深度范围是1.0，而不是0, 如果为0, 那么采样点永远处在阴影中.

   - 对于观察空间的边缘: 把深度贴图的纹理环绕选项设置为GL_CLAMP_TO_BORDER, 并指定边缘值为1.0.

```c
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
GLfloat borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
```

   - 对于观察空间的远平面, 在片段着色器中修改

```GLSL
    if(projCoords.z > 1.0)
        shadow = 0.0;
```

## 阴影的锯齿

深度贴图拥有固定分辨率, 多个片段对应一个纹理像素.

- 解决方案

PCF（percentage-closer filtering）

一共采样9个点(包括边缘的8个点), 对它们的shadow求平均值.

比较重要的是, 我们需要知道纹素texel的尺寸: 一个片段包含多少个像素

```GLSL
vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
```

这个textureSize返回一个给定采样器纹理的0级mipmap的vec2类型的宽和高。用1除以它返回一个单独纹理像素的大小，我们用以对纹理坐标进行偏移，确保每个新样本，来自不同的深度值。

## 透视投影

会进行提前深度测试, 深度值非线性.

如果要将正射投影改为透视投影, 住主要改动2处:

1. 渲染循环中投影矩阵生成
2. 片段着色器中将提前深度测试得到的深度值还原成线性

## 点光阴影 point light

可以用来唬人的名字 **万向阴影贴图（omnidirectional shadow maps）技术**

其实是利用Cubemap做出6个方向的深度贴图, 注意:

1. 1个光源渲染指定6个不同的视图矩阵
2. 一共渲染6次

**这很耗费性能!**

- 解决方案

几何着色器允许我们使用一次渲染过程来建立深度立方体贴图。

初始化是差不多的: 创建立方体贴图, 绑定到帧缓冲上.

使用流程也是差不多的: 1. 生成深度贴图. 2. 正常渲染, 利用深度图创建阴影.

