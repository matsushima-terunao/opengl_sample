blog  
https://mappuri.com/program/opengl-sample01-settings/  
https://mappuri.com/program/opengl-sample02-refactoring/  
https://mappuri.com/program/opengl-sample03-read_mesh/  
https://mappuri.com/program/opengl-sample04-texture-mapping/  
https://mappuri.com/program/opengl-sample05-render_text/  
https://mappuri.com/program/opengl-sample06-lighting/  
https://mappuri.com/program/opengl-sample07-fractal_texture/  

opengl_sample
====

OpenGL サンプルプログラム

# サンプル1

glfw のサンプルを動作させる

## xcode プロジェクト作成
- Create a new Xcode project
  - macOS -> Application -> Command Line Tool -> Next
  - Product Name: opengl_sample, Language: C++, あとは任意で -> Next
    - 任意のフォルダ -> Create
- Project navigator -> 左上の opengl_sample フォルダー 右クリック -> New File...
  - macOS -> C++ File -> Next
  - Name: sample01.cpp, [ ] Also create a header file -> Next
  - こちらのサンプルをコピペ
    - https://www.glfw.org/docs/latest/quick.html

## glfw インストール (macos homebrew でインストールする場合)
### homebrew で glfw インストール
    $ brew install glfw
    Updating Homebrew...
    ==> Auto-updated Homebrew!
    Updated 2 taps (homebrew/core and homebrew/cask).
    ==> New Formulae
    autorestic                  gcc@10                      imath                       lm-sensors                  nox                         openexr@2                   pgxnclient                  scotch                      sysstat                     virtualenvwrapper
    ddcctl                      gtksourceview5              ki                          lsix                        oksh                        openj9                      qodem                       seqkit                      tssh                        xfig
    ==> Updated Formulae
    Updated 751 formulae.
    ==> Deleted Formulae
    aurora-cli
    ==> New Casks
    crescendo                   depthmapx                   ilspy                       irpf2021                    mixed-in-key                pronterface                 sbarex-qlmarkdown           siyuan                      tabtopus                    vitals
    cryptonomic-galleon         hancom-word                 invoker                     jellyfin-media-player       mutesync                    recut                       shield                      specter                     usr-sse2-rdm
    ==> Updated Casks
    Updated 609 casks.
    ==> Deleted Casks
    adventure                cliqz                    duckstation              hubic                    meshcommander            netbeans-java-ee         nndd                     revisions                swift-explorer           transmit-disk            wakeonlan
    clipbuddy                cuteclips                family-tree-builder      lingo                    netbeans-cpp             netbeans-java-se         printrun                 spideroak-share          tracks-live              vrep
    
    ==> Downloading https://ghcr.io/v2/homebrew/core/glfw/manifests/3.3.4
    ######################################################################## 100.0%
    ==> Downloading https://ghcr.io/v2/homebrew/core/glfw/blobs/sha256:fb4c73abb6b230ffc2cacf187114584a1e589e67f399b78a56396911b2e1b483
    ==> Downloading from https://pkg-containers.githubusercontent.com/ghcr1/blobs/sha256:fb4c73abb6b230ffc2cacf187114584a1e589e67f399b78a56396911b2e1b483?se=2021-05-10T04%3A50%3A00Z&sig=5141Scvs8zfctRRmIdyp4zfyxdOL9UbL3KKDGA4Pwm8%3D&sp=r&spr=https&sr=b&sv=2019-12-12
    ######################################################################## 100.0%
    ==> Pouring glfw--3.3.4.mojave.bottle.tar.gz
    🍺  /usr/local/Cellar/glfw/3.3.4: 14 files, 495KB

### glfw パス確認

    $ brew info glfw
    glfw: stable 3.3.4 (bottled), HEAD
    Multi-platform library for OpenGL applications
    https://www.glfw.org/
    /usr/local/Cellar/glfw/3.3.4 (14 files, 495KB) *
      Poured from bottle on 2021-05-10 at 13:42:10
    From: https://github.com/Homebrew/homebrew-core/blob/HEAD/Formula/glfw.rb
    License: Zlib
    ==> Dependencies
    Build: cmake ✘
    ==> Options
    --HEAD
        Install HEAD version
    ==> Analytics
    install: 5,218 (30 days), 15,326 (90 days), 40,064 (365 days)
    install-on-request: 4,621 (30 days), 13,587 (90 days), 35,883 (365 days)
    build-error: 0 (30 days)

/usr/local/opt/glfw/include
/usr/local/opt/glfw/lib



### xcode プロジェクトに glfw パス、ライブラリ追加
- 左上の opengl_sample をクリック
  - Build Settings -> All -> Search Paths -> Header Search Paths -> /usr/local/Cellar/glfw/3.3.4/include, non-recursive
  - Build Phases -> Link Binary With Libraries
    - +マーク -> OpenGL.framework -> Add
    - /usr/local/Cellar/glfw/3.3.4/lib/libglfw.dylib をドロップ

## glfw インストール (glfw サイトからヘッダー、ライブラリをダウンロードする場合)

### glfw バイナリーダウンロード
- https://www.glfw.org/download.html
  - 64-bit macOS binaries をダウンロード
  - glfw-3.3.4.bin.MACOS.zip を展開して include, lib-universal フォルダを opengl_sample/glfw 配下に移動

### xcode プロジェクトに glfw パス、ライブラリ追加
- 左上の opengl_sample をクリック
  - Build Settings -> All -> Search Paths -> Header Search Paths -> opengl_sample/glfw/include, non-recursive
  - Build Phases -> Link Binary With Libraries
    - +マーク -> OpenGL.framework -> Add
    - opengl_sample/lib-universal/libglfw.3.dylib をドロップ

## xcode プロジェクトに glad パス、ファイル追加

- glad ソース生成
  - https://github.com/Dav1dde/glad
  - Glad 2 webservice -> gl Version 4.6, Core -> GENERATE
- glad フォルダをプロジェクト配下にコピー
- インクルードパス追加: opengl_sample/glad/include, non-recursive
- glfw Source package をダウンロードし、dep/linmath.h を opengl_sample 配下にコピー
  - https://www.glfw.org/download.html
- ソースファイル追加: glad/src/glad.c
- ソースファイル修正: sample01.cpp
```c
//#include <glad/gl.h>
#include <glad/glad.h>

    //gladLoadGL(glfwGetProcAddress);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

 //int main(void)
 int sample01(void)
```
- ソースファイル修正: main.cpp
```c
int sample01();

int main(int argc, const char * argv[]) {
    sample01();
    return 0;
}
```

# サンプル2

gl ラッパー関数作成。

## エラー処理
- glGetError
  - OpenGL のエラー取得
- atexit
  - プロセス正常終了時の関数登録
- glfwSetErrorCallback
  - glfw のエラーコールバック登録
- glDebugMessageCallback
  - OpenGL のデバッグメッセージコールバック登録

## メッシュ作成。
- create_vertex_buffer

## シェーダー作成。
- create_shader

## UBO(uniform buffer object) 作成。
- create_uniform_buffer
        //glUniformMatrix4fv(mvp_location1, 1, GL_FALSE, (const GLfloat*) &vertex_uniform.modelview_matrix);
        //glUniformMatrix4fv(mvp_location2, 1, GL_FALSE, (const GLfloat*) &vertex_uniform.projection_matrix);

# サンプル3

obj ファイル読み込み。

Product -> Scheme -> Edit scheme -> Options -> Working Directory -> [v] Use custom working directory: $PROJECT_DIR/opengl_sample

mesh.cpp
/**
 * メッシュ読み込み。
 */
 void read_mesh(const char* path, std::vector<Vertex>& vertex_list);

cube.obj
http://www.opengl-tutorial.org/print/
bunny
https://www.prinmath.com/csci5229/OBJ/index.html

# サンプル4

テクスチャーマッピング。

- opencv

/*
$ pkg-config --cflags opencv4
-I/usr/local/Cellar/opencv/4.5.0_5/include/opencv4
$ pkg-config --libs opencv4
-L/usr/local/Cellar/opencv/4.5.0_5/lib -lopencv_gapi -lopencv_stitching -lopencv_alphamat -lopencv_aruco -lopencv_bgsegm -lopencv_bioinspired -lopencv_ccalib -lopencv_dnn_objdetect -lopencv_dnn_superres -lopencv_dpm -lopencv_highgui -lopencv_face -lopencv_freetype -lopencv_fuzzy -lopencv_hfs -lopencv_img_hash -lopencv_intensity_transform -lopencv_line_descriptor -lopencv_mcc -lopencv_quality -lopencv_rapid -lopencv_reg -lopencv_rgbd -lopencv_saliency -lopencv_sfm -lopencv_stereo -lopencv_structured_light -lopencv_phase_unwrapping -lopencv_superres -lopencv_optflow -lopencv_surface_matching -lopencv_tracking -lopencv_datasets -lopencv_text -lopencv_dnn -lopencv_plot -lopencv_videostab -lopencv_videoio -lopencv_viz -lopencv_xfeatures2d -lopencv_shape -lopencv_ml -lopencv_ximgproc -lopencv_video -lopencv_xobjdetect -lopencv_objdetect -lopencv_calib3d -lopencv_imgcodecs -lopencv_features2d -lopencv_flann -lopencv_xphoto -lopencv_photo -lopencv_imgproc -lopencv_core

/usr/local/Cellar/opencv recursive
/usr/local/Cellar/opencv/4.5.0_5/lib
opencv_core.dylib
opencv_imgcodecs.dylib
*/
IT-04621-M:lib terunao.matsushima$ brew install opencv
Warning: opencv 4.5.3_1 is already installed and up-to-date.
To reinstall 4.5.3_1, run:
  brew reinstall opencv

IT-04621-M:lib terunao.matsushima$ pkg-config --cflags opencv4
-I/usr/local/opt/opencv/include/opencv4

IT-04621-M:lib terunao.matsushima$ pkg-config --libs opencv4
-L/usr/local/opt/opencv/lib -lopencv_gapi -lopencv_stitching -lopencv_alphamat -lopencv_aruco -lopencv_barcode -lopencv_bgsegm -lopencv_bioinspired -lopencv_ccalib -lopencv_dnn_objdetect -lopencv_dnn_superres -lopencv_dpm -lopencv_face -lopencv_freetype -lopencv_fuzzy -lopencv_hfs -lopencv_img_hash -lopencv_intensity_transform -lopencv_line_descriptor -lopencv_mcc -lopencv_quality -lopencv_rapid -lopencv_reg -lopencv_rgbd -lopencv_saliency -lopencv_sfm -lopencv_stereo -lopencv_structured_light -lopencv_phase_unwrapping -lopencv_superres -lopencv_optflow -lopencv_surface_matching -lopencv_tracking -lopencv_highgui -lopencv_datasets -lopencv_text -lopencv_plot -lopencv_videostab -lopencv_videoio -lopencv_viz -lopencv_wechat_qrcode -lopencv_xfeatures2d -lopencv_shape -lopencv_ml -lopencv_ximgproc -lopencv_video -lopencv_dnn -lopencv_xobjdetect -lopencv_objdetect -lopencv_calib3d -lopencv_imgcodecs -lopencv_features2d -lopencv_flann -lopencv_xphoto -lopencv_photo -lopencv_imgproc -lopencv_core
# サンプル5

テキスト描画。

IT-04621-M:freetype-2.10.4 terunao.matsushima$ brew install freetype

$ pkg-config --cflags freetype2
-I/usr/local/opt/freetype/include/freetype2
$ pkg-config --libs freetype2
-L/usr/local/opt/freetype/lib -lfreetype

/usr/local/opt/freetype/include/freetype2 non-recursive
/usr/local/opt/freetype/lib
libfreetype.dylib

render_text.cpp
 * テキスト描画初期化。
int init_render_text();

 * テキスト描画。
float render_text(const char* text, float x, float y, float scale, float color[3], bool proportional);

 * テキスト一覧描画。
float render_text_list(std::vector<std::pair<std::string, float*>>& render_text_listv, float x, float y, float scale, bool proportional);

 * テキスト追加。
void append_render_text(std::vector<std::pair<std::string, float*>>& render_text_list, const char* text, float* color);

 * パラメーター変更UI。
static void change_params();

LearnOpenGL - Text Rendering
https://learnopengl.com/In-Practice/Text-Rendering
https://learnopengl.com/code_viewer_gh.php?code=src/7.in_practice/2.text_rendering/text_rendering.cpp

The FreeType Project
https://www.freetype.org/

# サンプル6

ライティング。

Clockworkcoders Tutorials
https://www.opengl.org/sdk/docs/tutorials/ClockworkCoders/lighting.php
WebGL - Phong Shading
http://www.cs.toronto.edu/~jacobson/phong-demo/


sphere.obj
http://web.mit.edu/djwendel/www/weblogo/shapes/basic-shapes/sphere/sphere.obj

# サンプル7

フラクタルで地形画像生成。

# サンプル8

球体メッシュ生成。



