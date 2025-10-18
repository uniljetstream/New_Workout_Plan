# LVGL UI 개발 가이드

LCD에 원하는 화면을 만들고 출력하는 방법을 단계별로 설명합니다.

---

## 1. 어떤 코드를 봐야 하는가?

### 시작점: main.c의 UI 생성 부분

**[main/main.c:236-238](main/main.c#L236-L238)** - 이 부분이 UI를 만드는 곳입니다.

```c
// 화면 중앙에 텍스트 라벨 생성
lv_obj_t *label = lv_label_create(lv_scr_act());
lv_label_set_text(label, "Hello ESP-IDF and LVGL!");
lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
```

이 부분을 수정하거나 교체하여 원하는 UI를 만들 수 있습니다.

---

## 2. LVGL 기본 개념

### LVGL 객체(Object) 시스템

- **모든 UI 요소는 객체(lv_obj)**입니다
- 부모-자식 구조로 구성됩니다
- `lv_scr_act()` = 현재 활성 화면 (최상위 부모)

### 기본 구조

```c
// 1. 객체 생성
lv_obj_t *widget = lv_위젯이름_create(부모);

// 2. 속성 설정
lv_위젯이름_set_속성(widget, 값);

// 3. 위치/크기 조정
lv_obj_align(widget, 정렬방식, x오프셋, y오프셋);
```

---

## 3. 간단한 예제 - 버튼 만들기

[main.c:236-238](main/main.c#L236-L238)을 다음으로 교체해보세요:

```c
// 버튼 생성
lv_obj_t *btn = lv_btn_create(lv_scr_act());
lv_obj_set_size(btn, 120, 50);  // 너비 120, 높이 50
lv_obj_align(btn, LV_ALIGN_CENTER, 0, 0);  // 중앙 정렬

// 버튼 안에 텍스트 추가
lv_obj_t *label = lv_label_create(btn);
lv_label_set_text(label, "Click Me!");
lv_obj_center(label);  // 버튼 내부 중앙
```

---

## 4. 주요 LVGL 위젯들

### 기본 위젯

| 위젯 | 생성 함수 | 용도 |
|------|-----------|------|
| 라벨 | `lv_label_create()` | 텍스트 표시 |
| 버튼 | `lv_btn_create()` | 클릭 가능한 버튼 |
| 이미지 | `lv_img_create()` | 이미지 표시 |
| 슬라이더 | `lv_slider_create()` | 값 조절 |
| 스위치 | `lv_switch_create()` | ON/OFF 토글 |
| 체크박스 | `lv_checkbox_create()` | 체크박스 |
| 바 | `lv_bar_create()` | 진행 바 |

### 컨테이너 위젯

| 위젯 | 생성 함수 | 용도 |
|------|-----------|------|
| 패널 | `lv_obj_create()` | 기본 컨테이너 |
| 스크린 | `lv_scr_act()` | 전체 화면 |

---

## 5. 실용 예제 - 온도 표시 화면

```c
// ========================================================================
// 7. UI 생성 예제 - 온도 표시 화면
// ========================================================================

// 배경 컨테이너 생성
lv_obj_t *container = lv_obj_create(lv_scr_act());
lv_obj_set_size(container, 200, 150);
lv_obj_align(container, LV_ALIGN_CENTER, 0, 0);
lv_obj_set_style_bg_color(container, lv_color_hex(0x2196F3), 0);  // 파란색 배경

// 제목 라벨
lv_obj_t *title = lv_label_create(container);
lv_label_set_text(title, "Temperature");
lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
lv_obj_set_style_text_color(title, lv_color_white(), 0);

// 온도 값 라벨 (큰 글씨)
lv_obj_t *temp = lv_label_create(container);
lv_label_set_text(temp, "25.5");
lv_obj_align(temp, LV_ALIGN_CENTER, 0, 0);
lv_obj_set_style_text_font(temp, &lv_font_montserrat_48, 0);  // 큰 폰트
lv_obj_set_style_text_color(temp, lv_color_white(), 0);

// 단위 라벨
lv_obj_t *unit = lv_label_create(container);
lv_label_set_text(unit, "°C");
lv_obj_align(unit, LV_ALIGN_BOTTOM_MID, 0, -10);
lv_obj_set_style_text_color(unit, lv_color_white(), 0);
```

---

## 6. 이벤트 처리 - 버튼 클릭 반응

### 이벤트 콜백 함수 정의

```c
// 버튼 클릭 이벤트 콜백 함수
static void button_event_cb(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);
    lv_obj_t *label = lv_obj_get_child(btn, 0);

    static int count = 0;
    count++;

    char buf[32];
    sprintf(buf, "Clicked: %d", count);
    lv_label_set_text(label, buf);

    ESP_LOGI("UI", "버튼 클릭됨! 횟수: %d", count);
}
```

### UI 생성 및 이벤트 등록

```c
// 버튼 생성
lv_obj_t *btn = lv_btn_create(lv_scr_act());
lv_obj_set_size(btn, 120, 50);
lv_obj_align(btn, LV_ALIGN_CENTER, 0, 0);

// 버튼 라벨
lv_obj_t *label = lv_label_create(btn);
lv_label_set_text(label, "Click: 0");
lv_obj_center(label);

// 이벤트 핸들러 등록
lv_obj_add_event_cb(btn, button_event_cb, LV_EVENT_CLICKED, NULL);
```

---

## 7. 주요 LVGL 함수 정리

### 객체 생성 및 배치

```c
// 객체 생성
lv_obj_t *obj = lv_obj_create(parent);

// 크기 설정
lv_obj_set_size(obj, width, height);
lv_obj_set_width(obj, width);
lv_obj_set_height(obj, height);

// 위치 설정 (절대 좌표)
lv_obj_set_pos(obj, x, y);
lv_obj_set_x(obj, x);
lv_obj_set_y(obj, y);

// 정렬 (상대 위치)
lv_obj_align(obj, LV_ALIGN_CENTER, x_offset, y_offset);
lv_obj_center(obj);  // 부모 중앙
```

### 정렬 상수

```c
LV_ALIGN_CENTER          // 중앙
LV_ALIGN_TOP_LEFT        // 왼쪽 상단
LV_ALIGN_TOP_MID         // 중앙 상단
LV_ALIGN_TOP_RIGHT       // 오른쪽 상단
LV_ALIGN_BOTTOM_LEFT     // 왼쪽 하단
LV_ALIGN_BOTTOM_MID      // 중앙 하단
LV_ALIGN_BOTTOM_RIGHT    // 오른쪽 하단
LV_ALIGN_LEFT_MID        // 중앙 왼쪽
LV_ALIGN_RIGHT_MID       // 중앙 오른쪽
```

### 스타일 설정

```c
// 배경색
lv_obj_set_style_bg_color(obj, lv_color_hex(0xFF0000), 0);  // 빨간색

// 텍스트 색상
lv_obj_set_style_text_color(obj, lv_color_white(), 0);

// 폰트
lv_obj_set_style_text_font(obj, &lv_font_montserrat_48, 0);

// 테두리
lv_obj_set_style_border_width(obj, 2, 0);
lv_obj_set_style_border_color(obj, lv_color_hex(0x000000), 0);

// 패딩
lv_obj_set_style_pad_all(obj, 10, 0);  // 모든 방향 10px
```

### 색상 생성

```c
lv_color_hex(0xFF0000)      // RGB Hex 값
lv_color_white()            // 흰색
lv_color_black()            // 검정색
lv_palette_main(LV_PALETTE_RED)  // 팔레트 색상
```

---

## 8. 학습 로드맵

### 1단계: 기본 위젯 실험 (1-2일)
- 라벨, 버튼, 슬라이더 만들어보기
- 위치와 크기 조정 연습
- 색상과 폰트 변경

### 2단계: 레이아웃 구성 (1-2일)
- 여러 위젯을 배치하여 화면 구성
- 컨테이너 사용하기
- 정렬 연습

### 3단계: 이벤트 처리 (1일)
- 버튼 클릭 감지
- 슬라이더 값 변경 감지
- 터치 이벤트 처리

### 4단계: 실시간 데이터 표시 (1-2일)
- 센서 값을 읽어서 화면에 업데이트
- 타이머를 사용한 주기적 업데이트
- 동적 UI 변경

### 5단계: 복잡한 UI 구현
- 여러 화면 전환
- 애니메이션
- 커스텀 위젯

---

## 9. 공부해야 할 것들

### 필수 (우선순위 높음)

#### 1. LVGL 기본 위젯 사용법
- 공식 문서: https://docs.lvgl.io/8.4/widgets/
- 각 위젯의 생성, 속성 설정, 이벤트 처리

#### 2. 좌표와 정렬
- `lv_obj_align()` - 상대 위치 지정
- `lv_obj_set_pos()` - 절대 위치 지정
- `lv_obj_set_size()` - 크기 설정

#### 3. 스타일 (색상, 폰트)
- `lv_obj_set_style_bg_color()` - 배경색
- `lv_obj_set_style_text_color()` - 글자색
- `lv_obj_set_style_text_font()` - 폰트

### 중급 (조금 더 배우면)

#### 4. 이벤트 시스템
- 터치, 클릭, 값 변경 이벤트 처리
- `lv_obj_add_event_cb()`

#### 5. 애니메이션
- `lv_anim_t` 구조체 사용
- 부드러운 화면 전환

#### 6. 레이아웃
- Flex 레이아웃
- Grid 레이아웃

---

## 10. 유용한 자료

### 공식 문서
- **LVGL 공식 문서**: https://docs.lvgl.io/8.4/
- **LVGL 예제 브라우저**: https://docs.lvgl.io/8.4/examples.html

### 로컬 예제
- **ESP-IDF LVGL 예제**: `components/lvgl/examples/` 폴더
- **LVGL 데모 코드**: `components/lvgl/demos/` 폴더

### 주요 섹션
- **위젯 문서**: https://docs.lvgl.io/8.4/widgets/index.html
- **스타일 가이드**: https://docs.lvgl.io/8.4/overview/style.html
- **이벤트 가이드**: https://docs.lvgl.io/8.4/overview/event.html

---

## 11. 실습 예제 모음

### 예제 1: 여러 버튼 배치

```c
// 상단 버튼
lv_obj_t *btn1 = lv_btn_create(lv_scr_act());
lv_obj_set_size(btn1, 100, 40);
lv_obj_align(btn1, LV_ALIGN_TOP_MID, 0, 10);

lv_obj_t *label1 = lv_label_create(btn1);
lv_label_set_text(label1, "Button 1");
lv_obj_center(label1);

// 하단 버튼
lv_obj_t *btn2 = lv_btn_create(lv_scr_act());
lv_obj_set_size(btn2, 100, 40);
lv_obj_align(btn2, LV_ALIGN_BOTTOM_MID, 0, -10);

lv_obj_t *label2 = lv_label_create(btn2);
lv_label_set_text(label2, "Button 2");
lv_obj_center(label2);
```

### 예제 2: 슬라이더와 라벨 연동

```c
// 전역 변수로 라벨 선언 (콜백에서 접근하기 위해)
static lv_obj_t *value_label;

// 슬라이더 값 변경 콜백
static void slider_event_cb(lv_event_t *e)
{
    lv_obj_t *slider = lv_event_get_target(e);
    int32_t value = lv_slider_get_value(slider);

    char buf[32];
    sprintf(buf, "Value: %d", (int)value);
    lv_label_set_text(value_label, buf);
}

// UI 생성
void create_slider_ui(void)
{
    // 슬라이더 생성
    lv_obj_t *slider = lv_slider_create(lv_scr_act());
    lv_obj_set_width(slider, 200);
    lv_obj_align(slider, LV_ALIGN_CENTER, 0, 0);
    lv_slider_set_range(slider, 0, 100);  // 0~100 범위
    lv_slider_set_value(slider, 50, LV_ANIM_OFF);  // 초기값 50

    // 값 표시 라벨
    value_label = lv_label_create(lv_scr_act());
    lv_label_set_text(value_label, "Value: 50");
    lv_obj_align(value_label, LV_ALIGN_CENTER, 0, -40);

    // 이벤트 등록
    lv_obj_add_event_cb(slider, slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
}
```

### 예제 3: 스위치와 LED 시뮬레이션

```c
static lv_obj_t *led_indicator;

// 스위치 이벤트 콜백
static void switch_event_cb(lv_event_t *e)
{
    lv_obj_t *sw = lv_event_get_target(e);

    if (lv_obj_has_state(sw, LV_STATE_CHECKED)) {
        // 스위치 ON
        lv_obj_set_style_bg_color(led_indicator, lv_color_hex(0x00FF00), 0);  // 초록색
        ESP_LOGI("UI", "스위치 ON");
    } else {
        // 스위치 OFF
        lv_obj_set_style_bg_color(led_indicator, lv_color_hex(0xFF0000), 0);  // 빨간색
        ESP_LOGI("UI", "스위치 OFF");
    }
}

// UI 생성
void create_switch_ui(void)
{
    // LED 인디케이터 (원형)
    led_indicator = lv_obj_create(lv_scr_act());
    lv_obj_set_size(led_indicator, 60, 60);
    lv_obj_align(led_indicator, LV_ALIGN_CENTER, 0, -40);
    lv_obj_set_style_radius(led_indicator, LV_RADIUS_CIRCLE, 0);  // 원형
    lv_obj_set_style_bg_color(led_indicator, lv_color_hex(0xFF0000), 0);  // 빨간색

    // 스위치
    lv_obj_t *sw = lv_switch_create(lv_scr_act());
    lv_obj_align(sw, LV_ALIGN_CENTER, 0, 40);

    // 이벤트 등록
    lv_obj_add_event_cb(sw, switch_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
}
```

---

## 12. 디버깅 팁

### 로그 출력
```c
ESP_LOGI("UI", "버튼이 클릭되었습니다");
ESP_LOGW("UI", "경고: 값이 범위를 벗어났습니다");
ESP_LOGE("UI", "에러: 객체 생성 실패");
```

### 일반적인 문제

#### 1. 화면에 아무것도 안 보임
- `lv_timer_handler()` 태스크가 실행 중인지 확인
- 객체가 화면 밖에 배치되었는지 확인
- 배경색과 객체색이 같은지 확인

#### 2. 터치가 반응 안 함
- `touchpad_read()` 콜백이 올바른지 확인
- 터치 좌표가 올바른지 로그 확인
- 터치 센서 초기화 성공 여부 확인

#### 3. 메모리 부족 에러
- `sdkconfig`에서 `CONFIG_LV_MEM_SIZE_KILOBYTES` 증가
- 큰 이미지나 폰트 사용 줄이기
- 사용하지 않는 객체 삭제: `lv_obj_del(obj)`

---

## 13. 다음 단계

이제 실제로 코드를 작성해보세요!

### 연습 과제

1. **입문**: 화면에 "Hello World" 라벨과 버튼 1개 만들기
2. **초급**: 슬라이더로 LED 밝기 조절 UI 만들기
3. **중급**: 온도/습도를 표시하는 대시보드 만들기
4. **고급**: 여러 화면을 전환할 수 있는 메뉴 시스템 만들기

### 질문이 있다면

- 만들고 싶은 UI가 있으면 구체적으로 설명해주세요
- 예제 코드가 필요하면 요청해주세요
- 에러가 발생하면 에러 메시지와 코드를 공유해주세요

**화이팅!** 🚀
