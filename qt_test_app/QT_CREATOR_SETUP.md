# Qt Creator에서 Qt5 프로젝트 열기 및 빌드하기

Qt6 Creator에서 Qt5 프로젝트를 열고 빌드하는 방법을 안내합니다.

## 방법 1: CMake 프로젝트로 열기 (권장)

### 1. Qt Creator 실행 및 프로젝트 열기

```bash
qtcreator
```

또는 명령줄에서:
```bash
qtcreator CMakeLists.txt
```

### 2. Qt Creator에서 프로젝트 열기
1. **File → Open File or Project** 선택
2. `qt_test_app/CMakeLists.txt` 파일 선택
3. **Open** 클릭

### 3. Kit 설정 (Configure Project)

프로젝트를 처음 열면 "Configure Project" 화면이 나타납니다.

#### Qt5 Kit이 있는 경우:
1. **Desktop Qt 5.15.3** (또는 유사한 이름) Kit 선택
2. **Configure Project** 클릭

#### Qt5 Kit이 없는 경우:
1. **Manage Kits** 클릭
2. 아래 "Qt5 Kit 수동 설정" 섹션 참조

### 4. 빌드 및 실행
1. **Ctrl + B** - 빌드
2. **Ctrl + R** - 실행

## 방법 2: qmake 프로젝트로 열기

### 1. Qt Creator에서 프로젝트 열기
1. **File → Open File or Project** 선택
2. `qt_test_app/qt_test_app.pro` 파일 선택
3. **Open** 클릭

### 2. Kit 설정
- Qt5 Kit 선택 (위와 동일)

### 3. 빌드 및 실행
- **Ctrl + B** - 빌드
- **Ctrl + R** - 실행

## Qt5 Kit 수동 설정

Qt Creator에 Qt5 Kit이 자동으로 인식되지 않는 경우:

### 1. Kits 설정 페이지 열기
- **Tools → Options → Kits** (또는 **Edit → Preferences → Kits**)

### 2. Qt Versions 추가
1. **Qt Versions** 탭 선택
2. **Add...** 클릭
3. qmake 경로 선택: `/usr/bin/qmake` 또는 `/usr/lib/x86_64-linux-gnu/qt5/bin/qmake`
4. 이름: **Qt 5.15.3 (System)**
5. **Apply** 클릭

qmake 경로 찾기:
```bash
which qmake
# 또는
find /usr -name qmake -type f 2>/dev/null
```

### 3. Compilers 확인
1. **Compilers** 탭 선택
2. GCC가 자동으로 감지되어 있어야 함
3. 없으면 **Add → GCC** 선택하고 `/usr/bin/g++` 지정

### 4. Kit 추가
1. **Kits** 탭 선택
2. **Add** 클릭
3. 다음 설정:
   - **Name**: Qt 5.15.3 Desktop
   - **Device type**: Desktop
   - **Device**: Local PC
   - **Compiler (C++)**: GCC (x86 64bit)
   - **Qt version**: Qt 5.15.3 (System)
   - **CMake Tool**: System CMake
4. **Apply** → **OK** 클릭

### 5. 프로젝트 재설정
1. Qt Creator에서 프로젝트를 다시 열기
2. 새로 만든 Kit 선택
3. **Configure Project** 클릭

## 빌드 문제 해결

### Qt MQTT 모듈이 없다는 오류

```bash
# Qt5 MQTT 설치
sudo apt-get update
sudo apt-get install libqt5mqtt5 libqt5mqtt5-dev

# 또는 소스에서 빌드
git clone https://github.com/qt/qtmqtt.git
cd qtmqtt
git checkout 5.15
qmake
make
sudo make install
```

### CMake가 Qt5를 찾지 못하는 경우

CMakeLists.txt에 Qt5 경로 명시:
```cmake
set(CMAKE_PREFIX_PATH "/usr/lib/x86_64-linux-gnu/cmake/Qt5")
```

또는 환境変수 설정:
```bash
export CMAKE_PREFIX_PATH=/usr/lib/x86_64-linux-gnu/cmake/Qt5:$CMAKE_PREFIX_PATH
```

### 빌드 디렉토리 정리

```bash
cd qt_test_app
rm -rf build
mkdir build
cd build
cmake ..
make
```

## 명령줄에서 빌드 (Qt Creator 없이)

### CMake 사용
```bash
cd qt_test_app
mkdir build
cd build
cmake ..
make
./qt_test_app
```

### qmake 사용
```bash
cd qt_test_app
qmake qt_test_app.pro
make
./qt_test_app
```

## 디버깅

### Debug 모드로 빌드
Qt Creator에서:
1. 왼쪽 하단 **Debug** 모드 선택 (Release 대신)
2. **Ctrl + B**로 빌드
3. **F5**로 디버그 실행

명령줄에서:
```bash
# CMake
cmake -DCMAKE_BUILD_TYPE=Debug ..
make

# qmake
qmake CONFIG+=debug
make
```

## Qt Creator 단축키

- **Ctrl + B**: 빌드
- **Ctrl + R**: 실행
- **F5**: 디버그 시작
- **F4**: .h ↔ .cpp 전환
- **Ctrl + K**: Locator (파일 빠르게 열기)
- **Ctrl + /**: 주석 토글
- **Ctrl + I**: 자동 들여쓰기
- **F2**: 심볼로 이동

## 추가 참고사항

### config.json 위치
빌드 후 실행 파일과 같은 디렉토리에 `config.json`이 있어야 합니다.

CMake는 자동으로 복사하지만, qmake의 경우 수동 복사:
```bash
cp config.json build/
```

### Qt Creator에서 실행 시 작업 디렉토리
Qt Creator에서 실행 시 작업 디렉토리가 빌드 디렉토리가 아닐 수 있습니다.

설정 방법:
1. **Projects** (왼쪽 패널)
2. **Run** 탭
3. **Working directory**: `%{buildDir}` 설정

## 문제가 계속되는 경우

1. Qt Creator 재시작
2. 프로젝트 닫고 다시 열기
3. `.user` 파일 삭제: `rm qt_test_app.pro.user`
4. 빌드 디렉토리 삭제 후 재빌드

## 지원

문제가 있으면:
- Qt Creator의 **Help → About Plugins**에서 CMake 플러그인 활성화 확인
- **Help → System Information**에서 Qt 버전 확인
- 터미널에서 `qmake --version` 실행하여 Qt5 확인
