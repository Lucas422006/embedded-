/*
 * DAY 13 - THỜI GIAN, TIMEOUT VÀ LẬP LỊCH KHÔNG CHẶN
 * Mức độ: trung bình khá. Có thể chia thành hai buổi, mỗi buổi 60 phút.
 * Bài 1-2: thời gian và timeout. Bài 3-4: chu kỳ và tích hợp hệ thống.
 *
 * ======================== KIẾN THỨC CẦN HIỂU ========================
 *
 * 1. Tick là một nhãn thời gian, không phải đồng hồ vô hạn.
 *    Trong bài này, tick có kiểu uint32_t và đơn vị là 1 ms.
 *    Trên phần cứng thật, đơn vị tick phụ thuộc cấu hình timer/RTOS.
 *    Sau UINT32_MAX, tick quay về 0 (khoảng 49,7 ngày với tick 1 ms).
 *    Vì vậy không thể coi now có giá trị nhỏ hơn start là chưa đến hạn.
 *
 * 2. Phải đo KHOẢNG THỜI GIAN đã trôi qua, kể cả khi tick quay vòng.
 *    Ví dụ: start = UINT32_MAX - 5, sau đó now = 4: đã trôi qua 10 ms.
 *    Số nguyên unsigned 32-bit có phép tính theo modulo 2^32.
 *    Hãy vận dụng tính chất này; không dùng so sánh signed để đo khoảng
 *    thời gian, không dùng float, và không cộng deadline rồi so sánh trực tiếp.
 *    Đến đúng hạn cũng phải được xử lý; không chỉ xử lý khi đã quá hạn.
 *
 * 3. Giới hạn của mô hình.
 *    Mọi now được truyền vào đều đi theo thời gian thực tăng dần.
 *    Khoảng thời gian thực từ mốc đang lưu tới now phải nhỏ hơn 2^32 ms.
 *    Hai nhãn uint32_t không thể cho biết đã đi qua nhiều vòng quay.
 *    Bài này giới hạn duration/period không quá INT32_MAX ms.
 *    Duration = 0 được phép: timeout tức thời. Period = 0 không hợp lệ.
 *
 * 4. Không chặn nghĩa là gì?
 *    Mỗi lần Poll chỉ quan sát trạng thái, cập nhật rồi trả về ngay.
 *    Không while chờ đồng hồ, không sleep/delay, không tự tăng now.
 *    Vòng lặp chính gọi Poll nhiều lần trong lúc vẫn làm các việc khác.
 *    Poll muộn thì sự kiện được phát hiện muộn; đây không phải bảo đảm
 *    một tác vụ sẽ chạy chính xác ở từng mili-giây trên hệ thống thật.
 *
 * 5. Timeout một lần và timer chu kỳ khác nhau.
 *    Timeout đã báo hết hạn thì tắt, chỉ hoạt động lại khi được Arm.
 *    Timer chu kỳ vẫn chạy và phải nhớ các mốc kế hoạch ban đầu.
 *    Ví dụ chu kỳ 10 ms, bắt đầu ở 100: các mốc là 110,120,130,...
 *    Nếu tới 135 mới kiểm tra: có 3 kỳ đã đến, mốc kế tiếp vẫn là 140.
 *    Nếu lấy 135 làm mốc mới, lịch sẽ trôi sang 145: không đúng yêu cầu này.
 *
 * 6. Đếm kỳ bị trễ không tạo lại được mẫu cảm biến đã bỏ lỡ.
 *    sample_due = 3 nghĩa là đã qua 3 mốc lấy mẫu, không có nghĩa bạn
 *    đang giữ 3 mẫu dữ liệu thật. Chính sách xử lý dữ liệu trễ là việc khác.
 *
 * CÁCH LÀM
 * - Chỉ hoàn thiện 5 hàm có TODO. Có thể thêm hàm phụ nếu cần.
 * - Không sửa hợp đồng hoặc các test có sẵn để làm cho kết quả PASS.
 * - Không malloc; không dùng biến toàn cục lưu trạng thái timer.
 * - Không có vòng lặp chạy theo số mili-giây hoặc số kỳ bị trễ.
 * - Các test có sẵn là mức kiểm tra tối thiểu; tự thêm ít nhất 2 test.
 * - Chưa có lời giải nên bản gốc có test FAIL là bình thường.
 *
 * BUILD từ thư mục embedded:
 * gcc -std=c11 -Wall -Wextra -Wpedantic -g exercises/day13_timing_nonblocking.c -o exercises/day13.exe
 * RUN (PowerShell): .\exercises\day13.exe
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define TIMER_MAX_DURATION ((uint32_t)INT32_MAX)

typedef struct {
    uint32_t started_at;
    uint32_t duration;
    bool armed;
} Timeout;

typedef struct {
    uint32_t anchor;
    uint32_t period;
    bool enabled;
} PeriodicTimer;

typedef struct {
    PeriodicTimer sampling;
    PeriodicTimer reporting;
    Timeout receive_guard;
} NodeTimers;

typedef struct {
    uint32_t sample_due;
    uint32_t report_due;
    bool receive_timeout;
} NodeEvents;

/*
 * BÀI 1 - KIỂM TRA KHOẢNG THỜI GIAN
 * Trả true khi từ start tới now đã đủ interval ms.
 * interval = 0: true. interval > TIMER_MAX_DURATION: false.
 * Không có trạng thái để thay đổi. Phải xử lý được tick quay vòng.
 */
bool tick_has_elapsed(uint32_t start, uint32_t now, uint32_t interval)
{
    /* TODO 1 */
     if( interval == 0 ) return true;
    else if ( interval > TIMER_MAX_DURATION ) return false;
     else if( now >= start ){
    if( ( now - start) >= interval ) return true;
    } else if ( now <= start ){
      if( ( (uint32_t)now - (uint32_t)start  ) >= interval ) return true;
    }
    return false;
}

/*
 * BÀI 2A - KHỞI ĐỘNG / KHỞI ĐỘNG LẠI TIMEOUT
 * Thành công: lưu mốc now, duration, bật armed và trả true.
 * Arm lại khi đang chạy: thay thế toàn bộ lần hẹn cũ.
 * timer == NULL hoặc duration quá giới hạn: false, không sửa timer.
 * duration = 0 hợp lệ. Không phát sự kiện trong Arm; Poll sẽ phát.
 */
bool timeout_arm(Timeout *timer, uint32_t now, uint32_t duration)
{
    /* TODO 2A */
    if( timer == NULL || duration > INT32_MAX ) return false;
    timer->duration = duration;
    timer->started_at = now;
    timer->armed = true;
    return true;
}

/*
 * BÀI 2B - THĂM DÒ TIMEOUT MỘT LẦN
 * timer NULL/chưa armed: false. Chưa đủ thời gian: false, giữ nguyên.
 * Đủ thời gian: true đúng một lần và tắt armed.
 * Giữ nguyên started_at và duration khi Poll.
 * Tái sử dụng tick_has_elapsed; không chép lại thuật toán đo thời gian.
 * Caller chỉ đưa timer đã khởi tạo, có duration hợp lệ, vào Poll.
 */
bool timeout_poll(Timeout *timer, uint32_t now)
{
    /* TODO 2B */
    if( timer == NULL) return false;
    if( timer->armed == false ) return false;
    bool time =  tick_has_elapsed( timer->started_at, now, timer->duration);
    if( time == false ){
      return false;
    } else { 
      timer->armed = false;
      return true;
    }
}

/*
 * BÀI 3 - ĐẾM CÁC KỲ ĐÃ ĐẾN, GIỮ NGUYÊN PHA LỊCH
 * Trả số mốc chu kỳ đã đến kể từ anchor, đồng thời cập nhật anchor
 * tới mốc kế hoạch gần nhất đã đi qua. Không đặt anchor = now nếu
 * now không nằm đúng một mốc kế hoạch.
 *
 * Ví dụ độc lập: anchor=100, period=10, enabled=true.
 * Poll ở 109 -> 0 kỳ, anchor vẫn 100.
 * Poll ở 135 -> 3 kỳ, anchor thành 130.
 * Poll lại ở 135 -> 0 kỳ. Poll ở 140 -> 1 kỳ, anchor thành 140.
 *
 * timer NULL, disabled, period=0 hoặc period quá giới hạn:
 * trả 0 và không sửa timer. Không thay đổi period/enabled.
 * Phải xử lý được hàng triệu kỳ trễ bằng số phép tính cố định O(1).
 */
uint32_t periodic_collect_due(PeriodicTimer *timer, uint32_t now)
{
    /* TODO 3 */
    if( timer == NULL || timer->enabled == false || timer->period == 0 || timer->period > INT32_MAX ) return 0;
    uint32_t Periodic = 0;
    if( now >= timer->anchor ){
     Periodic = ( now - timer->anchor )/timer->period;
    if ( Periodic != 0 ) {
      timer->anchor += Periodic*timer->period;
    } }else {
      Periodic = ((uint32_t) now - (uint32_t )timer->anchor )/timer->period;
      if ( Periodic != 0 ) {
      timer->anchor += Periodic*timer->period;}
} 
    return Periodic;
}

/* Phần khởi tạo hệ thống được cung cấp, không phải bài tập. */
static void node_init(NodeTimers *node, uint32_t now)
{
    if (node != NULL) {
        node->sampling = (PeriodicTimer){now, 10U, true};
        node->reporting = (PeriodicTimer){now, 50U, true};
        node->receive_guard = (Timeout){now, 35U, true};
    }
}

/*typedef struct {
    uint32_t started_at;
    uint32_t duration;
    bool armed;
} Timeout;

typedef struct {
    uint32_t anchor;
    uint32_t period;
    bool enabled;
} PeriodicTimer;

typedef struct {
    PeriodicTimer sampling;
    PeriodicTimer reporting;
    Timeout receive_guard;
} NodeTimers;

typedef struct {
    uint32_t sample_due;
    uint32_t report_due;
    bool receive_timeout;
} NodeEvents; /*
/*
 * BÀI 4 - GHÉP THÀNH NODE CẢM BIẾN KHÔNG CHẶN
 * Node có lịch lấy mẫu 10 ms, báo cáo 50 ms, timeout nhận dữ liệu 35 ms.
 * Trả sample_due/report_due bằng cách dùng periodic_collect_due.
 * receive_timeout dùng timeout_poll: mỗi lần mất dữ liệu chỉ báo 1 lần.
 *
 * Nếu valid_packet == true: Arm lại receive_guard tại now, duration=35,
 * TRƯỚC khi kiểm tra timeout. Như vậy gói hợp lệ đến đúng hạn sẽ được
 * chấp nhận, không phát timeout cũ. Timer đã tắt cũng được Arm trở lại.
 * valid_packet == false: không tự Arm lại timer đã hết hạn.
 *
 * node == NULL: trả ba trường đều 0/false.
 * Không đổi chu kỳ của sampling/reporting; không sửa lại các hàm trên
 * chỉ để xử lý riêng bài này. Không printf trong node_poll.
 */
NodeEvents node_poll(NodeTimers *node, uint32_t now, bool valid_packet)
{
    /* TODO 4 */
    if( node == NULL )  return (NodeEvents){0U, 0U, false}; 
    if( valid_packet == true ){
      node->receive_guard.armed = true;
      node->receive_guard.started_at = now;
      node->receive_guard.duration = 35;
    } 
      NodeEvents events;
      events.sample_due =  periodic_collect_due(&node->sampling,now);
      events.report_due =  periodic_collect_due(&node->reporting,now);
      events.receive_timeout =  timeout_poll(&node->receive_guard,now);
    return events;
}

/* ==================== BỘ KIỂM TRA CÓ SẴN ==================== */

typedef struct {
    unsigned int passed;
    unsigned int failed;
} TestResults;

static void check(TestResults *result, bool condition, const char *name)
{
    printf("[%s] %s\n", condition ? "PASS" : "FAIL", name);
    if (condition) {
        ++result->passed;
    } else {
        ++result->failed;
    }
}

static bool timeout_equals(const Timeout *timer, uint32_t start,
                           uint32_t duration, bool armed)
{
    return timer->started_at == start && timer->duration == duration &&
           timer->armed == armed;
}

static bool events_equal(NodeEvents event, uint32_t samples,
                         uint32_t reports, bool timeout)
{
    return event.sample_due == samples && event.report_due == reports &&
           event.receive_timeout == timeout;
}

static void test_elapsed(TestResults *result)
{
    check(result, !tick_has_elapsed(1000U, 1049U, 50U), "1.1: before deadline");
    check(result, tick_has_elapsed(1000U, 1050U, 50U), "1.2: exact deadline");
    check(result, tick_has_elapsed(1000U, 1087U, 50U), "1.3: late polling");
    check(result, tick_has_elapsed(55U, 55U, 0U), "1.4: zero duration");
    check(result, !tick_has_elapsed(UINT32_MAX - 5U, 3U, 10U),
          "1.5: wrap, one tick early");
    check(result, tick_has_elapsed(UINT32_MAX - 5U, 4U, 10U),
          "1.6: wrap, exact deadline");
    check(result, tick_has_elapsed(UINT32_MAX, 0U, 1U),
          "1.7: last tick to zero is one millisecond");
    check(result, tick_has_elapsed(0U, TIMER_MAX_DURATION, TIMER_MAX_DURATION),
          "1.8: maximum permitted duration");
    check(result, !tick_has_elapsed(0U, UINT32_MAX, TIMER_MAX_DURATION + 1U),
          "1.9: invalid duration");
}

static void test_timeout(TestResults *result)
{
    Timeout timer = {11U, 22U, false};
    bool ok = timeout_arm(&timer, 1000U, 50U);
    check(result, ok && timeout_equals(&timer, 1000U, 50U, true),
          "2.1: arm initializes all fields");

    timer = (Timeout){1000U, 50U, true};
    ok = timeout_poll(&timer, 1049U);
    check(result, !ok && timeout_equals(&timer, 1000U, 50U, true),
          "2.2: early poll preserves state");
    ok = timeout_poll(&timer, 1050U);
    check(result, ok && timeout_equals(&timer, 1000U, 50U, false),
          "2.3: exact deadline emits once and disarms");
    check(result, !timeout_poll(&timer, 1050U) && !timeout_poll(&timer, 1100U),
          "2.4: no repeated event");

    timer = (Timeout){2000U, 100U, true};
    ok = timeout_arm(&timer, 2030U, 10U);
    check(result, ok && timeout_equals(&timer, 2030U, 10U, true),
          "2.5: rearm replaces pending timeout");

    timer = (Timeout){UINT32_MAX - 5U, 10U, true};
    ok = timeout_poll(&timer, 4U);
    check(result, ok && !timer.armed && !timeout_poll(&timer, 4U),
          "2.6: one-shot expiry across wrap");

    timer = (Timeout){77U, 0U, true};
    ok = timeout_poll(&timer, 77U);
    check(result, ok && !timeout_poll(&timer, 77U), "2.7: immediate one-shot");

    timer = (Timeout){11U, 22U, true};
    ok = timeout_arm(&timer, 99U, TIMER_MAX_DURATION + 1U);
    check(result, !ok && timeout_equals(&timer, 11U, 22U, true),
          "2.8: invalid rearm leaves old state unchanged");
    ok = timeout_arm(&timer, 99U, 0U);
    check(result, ok && timeout_equals(&timer, 99U, 0U, true),
          "2.9: zero duration is accepted by arm");
    check(result, !timeout_arm(NULL, 0U, 1U) && !timeout_poll(NULL, 1U),
          "2.10: NULL inputs");
    timer = (Timeout){0U, 1U, false};
    check(result, !timeout_poll(&timer, 100U) && !timer.armed,
          "2.11: inactive timeout stays inactive");
}

static void test_periodic(TestResults *result)
{
    PeriodicTimer timer = {100U, 10U, true};
    uint32_t due = periodic_collect_due(&timer, 109U);
    check(result, due == 0U && timer.anchor == 100U,
          "3.1: no cycle due");
    due = periodic_collect_due(&timer, 135U);
    check(result, due == 3U && timer.anchor == 130U &&
                  timer.period == 10U && timer.enabled,
          "3.2: count three cycles, keep original phase");
    due = periodic_collect_due(&timer, 135U);
    check(result, due == 0U && timer.anchor == 130U,
          "3.3: same tick cannot count twice");
    due = periodic_collect_due(&timer, 140U);
    check(result, due == 1U && timer.anchor == 140U,
          "3.4: next deadline remains 140");

    timer = (PeriodicTimer){UINT32_MAX - 5U, 10U, true};
    due = periodic_collect_due(&timer, 29U);
    check(result, due == 3U && timer.anchor == 24U,
          "3.5: phase preserved across wrap");

    timer = (PeriodicTimer){0U, 1U, true};
    due = periodic_collect_due(&timer, 1000000000U);
    check(result, due == 1000000000U && timer.anchor == 1000000000U,
          "3.6: billion-cycle gap, O(1) required");

    timer = (PeriodicTimer){10U, 0U, true};
    due = periodic_collect_due(&timer, 100U);
    check(result, due == 0U && timer.anchor == 10U && timer.enabled,
          "3.7: zero period does not divide or mutate");
    timer = (PeriodicTimer){10U, TIMER_MAX_DURATION + 1U, true};
    due = periodic_collect_due(&timer, 100U);
    check(result, due == 0U && timer.anchor == 10U &&
                  timer.period == TIMER_MAX_DURATION + 1U && timer.enabled,
          "3.8: invalid period preserves state");
    timer = (PeriodicTimer){10U, 10U, false};
    due = periodic_collect_due(&timer, 100U);
    check(result, due == 0U && timer.anchor == 10U && !timer.enabled,
          "3.9: disabled periodic timer");
    check(result, periodic_collect_due(NULL, 100U) == 0U, "3.10: NULL input");
}

static void test_node(TestResults *result)
{
    NodeTimers node;
    NodeEvents event;
    node_init(&node, 1000U);

    event = node_poll(&node, 1000U, false);
    check(result, events_equal(event, 0U, 0U, false), "4.1: initial tick");
    event = node_poll(&node, 1010U, false);
    check(result, events_equal(event, 1U, 0U, false), "4.2: sample only");
    event = node_poll(&node, 1050U, false);
    check(result, events_equal(event, 4U, 1U, true),
          "4.3: late poll collects both schedules and timeout");
    event = node_poll(&node, 1050U, false);
    check(result, events_equal(event, 0U, 0U, false), "4.4: no duplicate events");
    event = node_poll(&node, 1051U, true);
    check(result, events_equal(event, 0U, 0U, false) &&
                  timeout_equals(&node.receive_guard, 1051U, 35U, true),
          "4.5: valid packet rearms an expired guard");
    event = node_poll(&node, 1085U, false);
    check(result, events_equal(event, 3U, 0U, false),
          "4.6: guard not yet expired after rearm");
    event = node_poll(&node, 1086U, false);
    check(result, events_equal(event, 0U, 0U, true), "4.7: rearmed guard expires");

    node_init(&node, 0U);
    event = node_poll(&node, 35U, true);
    check(result, events_equal(event, 3U, 0U, false) &&
                  timeout_equals(&node.receive_guard, 35U, 35U, true),
          "4.8: packet wins when arriving at the exact old deadline");
    event = node_poll(&node, 69U, false);
    check(result, events_equal(event, 3U, 1U, false),
          "4.9: independent schedule phases");
    event = node_poll(&node, 70U, false);
    check(result, events_equal(event, 1U, 0U, true), "4.10: new receive deadline");

    node_init(&node, UINT32_MAX - 5U);
    event = node_poll(&node, 29U, false);
    check(result, events_equal(event, 3U, 0U, true), "4.11: whole node across wrap");
    event = node_poll(&node, 44U, true);
    check(result, events_equal(event, 2U, 1U, false) && node.receive_guard.armed &&
                  node.sampling.period == 10U && node.reporting.period == 50U,
          "4.12: schedules continue after wrap and timeout");
    check(result, events_equal(node_poll(NULL, 10U, true), 0U, 0U, false),
          "4.13: NULL node");
}

static void print_summary(const TestResults *result)
{
    printf("Result: %u PASS, %u FAIL\n\n", result->passed, result->failed);
}

int main(void)
{
    TestResults results[4] = {{0U, 0U}, {0U, 0U}, {0U, 0U}, {0U, 0U}};
    unsigned int failures = 0U;

    puts("EXERCISE 1 - elapsed time");
    test_elapsed(&results[0]);
    print_summary(&results[0]);
    puts("EXERCISE 2 - one-shot timeout");
    test_timeout(&results[1]);
    print_summary(&results[1]);
    puts("EXERCISE 3 - periodic timer");
    test_periodic(&results[2]);
    print_summary(&results[2]);
    puts("EXERCISE 4 - sensor node");
    test_node(&results[3]);
    print_summary(&results[3]);

    for (unsigned int i = 0U; i < 4U; ++i) {
        failures += results[i].failed;
    }
    puts(failures == 0U ? "ALL TESTS PASSED" : "Some exercises still need work.");
    return failures == 0U ? 0 : 1;
}

/*
 * CÂU HỎI KHI NỘP BÀI
 * 1. Vì sao so sánh trực tiếp now với start + duration có thể sai?
 * 2. Vì sao Poll timer chu kỳ không luôn đặt anchor thành now?
 * 3. Nhận gói đúng hạn có được phát timeout không? Hợp đồng quyết định gì?
 * 4. sample_due = 5 có chứng minh đã thu được 5 mẫu thật không?
 * 5. Các test nào bạn tự thêm, và chúng phát hiện loại lỗi nào?
 */
