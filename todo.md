# TODO
- [2020/11/23 05:44]
    - search the optimal size for apps segment size (default was 64 KB)
        - 1024 KB : was OK, but too wasteful
        - 128 KB : was OK, seems everything is alright now (11/23 20:47)
        - 64 KB : unable to launch `color` (app)
        - part to edit: `SIZE_APPMEM` in `console.h` , and `app.ld`
    - try to make console as an app some day
        - need to understand how "privilege level" works
    - implement window resizing function
        - it may need to draw rectangle to represent the area of the window to be resized.
            - https://twitter.com/yosswi414_0/status/1330834570074148865
            - perhaps putting a transparent sheet that any of click passes through as the top and draw the rectangle on it will work well

- [2020/11/24 08:22]
    - progress: p.570 / day.27

# About this text

This text is made to remind me of some ideas about:

- part of bug, way of debug
- new implementation, function
- simplification of the entire structure
- apps

To maximize the effect of this document, it is preferred to show this text on the editor every time it launches.

# Reference which helped resolving some problems
- `-fno-delete-null-pointer-checks`
    - https://qiita.com/AoiMoe/items/2554f78dc9c197d22109
    - http://blog.kmckk.com/archives/1202810.html
