/*
 * DAY 12 — CNN ĐA KÊNH & LẬP LỊCH TÁC VỤ KHÔNG CHẶN
 * Mức trung bình | 60 phút: đọc 15, bài 1: 15, bài 2: 8, bài 3: 12, bài 4: 10.
 * Tiếp nối nội dung gần nhất ở trang 30–31 của PDF người học cung cấp.
 * Đây là bài tập mới được soạn theo lộ trình, không phải code trích từ PDF.
 *
 * KIẾN THỨC CẦN NẮM
 * 1. Dữ liệu đa kênh: tại mỗi thời điểm, cảm biến có nhiều giá trị đồng thời.
 *    Bài này có hai kênh X, Y. Bộ nhớ lưu xen kẽ theo thời gian:
 *    [X0,Y0, X1,Y1, X2,Y2, ...]. N là số thời điểm, không phải số byte.
 * 2. Conv đa kênh: một bộ lọc nhìn vào một đoạn thời gian của cả hai kênh.
 *    Mỗi giá trị đầu vào có trọng số tương ứng; các tích đóng góp vào cùng
 *    một tổng, rồi cộng bias. Hai kênh đầu vào vẫn có thể tạo MỘT kênh đầu ra.
 *    Bài này dùng kernel dài 3 thời điểm, valid, stride 1, giữ nguyên thứ tự
 *    kernel (quy ước cross-correlation thường được gọi là Conv trong ML).
 *    Tích lũy int32_t, sau đó ReLU và bão hòa về [0,127].
 *    Để vừa 1 giờ, bài này quy ước scale = 1, zero_point = 0, không requantize.
 *    Đây là mô hình số nguyên để học, chưa phải toán tử INT8 triển khai đầy đủ.
 * 3. Global max pooling: chọn giá trị lớn nhất của toàn bộ feature map.
 *    Đầu ra là điểm đặc trưng, không tự động là xác suất hoặc độ tin cậy.
 * 4. Scheduler hợp tác: vòng lặp chính thường xuyên kiểm tra tác vụ đã đến hạn
 *    chưa. Đến hạn thì gọi callback ngắn rồi trả quyền điều khiển. Không delay,
 *    không ngồi chờ đủ mẫu. Một callback dài vẫn làm các tác vụ khác phải đợi.
 *    uint32_t tick tính theo mili giây có thể quay từ UINT32_MAX về 0.
 *    Phải tính đúng thời gian đã trôi qua khi đi qua ranh giới đó.
 * 5. Pipeline có trạng thái: mỗi lần callback chỉ thử nhận MỘT cặp X,Y.
 *    Nếu đủ cửa sổ thì suy luận một lần và bắt đầu cửa sổ mới. Các mẫu chưa đủ
 *    được giữ trong struct giữa các lần gọi; không dùng malloc/free.
 *
 * QUY ƯỚC CHUNG
 * - Tự viết thuật toán trong 4 vùng TODO; được thêm hàm phụ nếu cần.
 * - Struct được cho sẵn để tập trung vào xử lý; không có lời giải từng bước.
 * - Không sửa phần TEST để đạt PASS. Chạy bản khung ban đầu sẽ có 0/4 PASS.
 * - Dùng C11; không cần board, HAL, FreeRTOS hoặc thư viện AI.
 * - Con trỏ không NULL phải trỏ đến vùng nhớ đủ dài theo hợp đồng mỗi hàm.
 * - Các vùng input, kernel, output phải tách biệt, không chồng lấn.
 * - Dữ liệu và hàm được gọi tuần tự trong main, không chia sẻ với ISR.
 *
 * Biên dịch (GCC):
 * gcc -std=c11 -Wall -Wextra -Werror day12_multichannel_scheduler.c -o day12
 * Chạy PowerShell: .\day12.exe    |    Linux/macOS: ./day12
 */

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>


/* BÀI 1 — CONV HAI KÊNH (15 phút)
 * input: n_frames cặp X,Y; kernel: đúng 3 cặp trọng số X,Y.
 * bias thuộc [-1000000,1000000]; n_frames hợp lệ trong [3,16].
 * Trả số phần tử đầu ra; lỗi trả 0 và KHÔNG ghi output.
 * Lỗi: con trỏ NULL, n_frames ngoài phạm vi, output_capacity không đủ.
 * Mỗi cửa sổ chỉ dùng các thời điểm có thật trong input, không padding.
 * Áp dụng cơ chế Conv, bias, ReLU, bão hòa đã mô tả ở trên.
 * Ví dụ input {1,10, 2,20, 3,30, 4,40}, kernel {1,0, 0,1, -1,0},
 * bias 0 -> output {18,28}. Cùng input, bias -20 -> {0,8}.
 */
uint8_t Conv2Ch(const int8_t *input, uint8_t n_frames,
                const int8_t *kernel, int32_t bias,
                int8_t *output, uint8_t output_capacity)
{
    /* TODO BÀI 1. Xóa các dòng giữ chỗ khi đã sử dụng tham số. */
    if( input == NULL || n_frames < 3 || n_frames > 16 || kernel == NULL || output == NULL || output_capacity < n_frames - 2) return 0;
    int n = 0;
    for( size_t i = 0; i <= (n_frames*2 - 6 ) ; i += 2 ){
        int32_t sum = 0;
        for( size_t k = 0; k < 6; k++){
            sum += input[i+ k]*kernel[k]; 
        } 
                sum += bias;
        if( sum < 0 ) sum = 0;
        else if( sum > 127 ) sum = 127;
        output[n] = sum;
        n++;
    }
    return (uint8_t)n;
}

/* BÀI 2 — GLOBAL MAX POOLING (8 phút)
 * Tìm giá trị lớn nhất trong input có len phần tử; chấp nhận cả số âm.
 * Thành công: ghi *output, trả true. Ví dụ {-8,-3,-9} -> -3.
 * NULL hoặc len ngoài [1,16]: trả false, không đổi output.
 * Đây là pooling trên toàn mảng, khác pooling từng khối của Day 11.
 */
bool GlobalMax(const int8_t *input, uint8_t len, int8_t *output)
{
    /* TODO BÀI 2. */
    if( input == NULL || len == 0 || output == NULL || len < 1 || len > 16 ) return false;
    int32_t max = input[0];
    for( size_t i = 0; i < len; i++){
        if( max < input[i] ) max = input[i];
    }
    *output = max;
    return true;
}

typedef void (*TaskFn)(void *ctx);
typedef struct {
    uint32_t last_ms;
    uint32_t period_ms;
    TaskFn callback;
    void *ctx;                 /* Được phép NULL; truyền nguyên vào callback. */
} Task_t;

/* BÀI 3 — TÁC VỤ ĐỊNH KỲ (12 phút)
 * Task_t được người gọi khởi tạo đầy đủ trước khi dùng.
 * Chưa đủ period_ms kể từ last_ms: không thay đổi task, trả false.
 * Đã đủ: cập nhật last_ms bằng now_ms TRƯỚC callback, gọi callback đúng
 * một lần với ctx và trả true. Trễ nhiều chu kỳ cũng không chạy bù.
 * task NULL, callback NULL hoặc period_ms == 0: trả false, không sửa task.
 * period_ms hợp lệ còn lại <= INT32_MAX; khoảng cách giữa các lần kiểm tra
 * nhỏ hơn 2^31 ms. Tick tăng theo thời gian và được phép quay vòng uint32_t.
 * Ví dụ last=100, period=10: now=109 chưa chạy; now=110 chạy;
 * sau đó now=145 chỉ chạy một lần, last trở thành 145.
 * Callback không sửa Task_t, không gọi lại Task_Run trên chính task đó.
 */
bool Task_init( Task_t *task){
    if( task == NULL  ||  task->callback == NULL ) return false;
    task->last_ms = 0;
    task->period_ms = 10;
    task->ctx = NULL;
    return true;
}

bool Task_Run(Task_t *task, uint32_t now_ms)
{
    /* TODO BÀI 3. */
    if(  task == NULL || task->callback == NULL || task->period_ms == 0) return false;
    if((now_ms - task->last_ms  ) < 10 ) return false;
    else {
        task->last_ms = now_ms;
        task->callback(task->ctx);
        return true;
    }
    
}

typedef bool (*ReadFrameFn)(void *ctx, int8_t frame[2]);
typedef void (*EmitScoreFn)(void *ctx, int8_t score);
typedef struct {
    int8_t window[8];          /* Bốn thời điểm, mỗi thời điểm hai kênh. */
    uint8_t count;             /* Số cặp đang có, khởi tạo 0; hợp lệ 0..3. */
    ReadFrameFn read;
    EmitScoreFn emit;
    void *io_ctx;
} AI_t;

static const int8_t AI_KERNEL[6] = {1,0, 0,1, -1,0};

/* BÀI 4 — GHÉP PIPELINE KHÔNG CHẶN (10 phút)
 * AI_Step là callback cho Task_Run, ctx trỏ đến AI_t đã khởi tạo đầy đủ.
 * Mỗi lần gọi chỉ thử read đúng một lần, lấy một cặp X,Y mới.
 * read trả false: không thêm mẫu, không suy luận, giữ nguyên count/window.
 * Nhận mẫu thành công: lưu theo thứ tự thời gian.
 * Đủ 4 cặp: gọi Conv2Ch với AI_KERNEL và bias 0, rồi GlobalMax;
 * phát đúng một score qua emit và đặt count về 0 cho cửa sổ tiếp theo.
 * Dùng bộ đệm có kích thước cố định; không phải cửa sổ chồng lấn.
 * ctx NULL, read NULL, emit NULL hoặc count > 3: không làm gì.
 * read/emit là callback đồng bộ, trả về nhanh; không sửa lại AI_t.
 * Không chờ mẫu, không delay, không cấp phát động; tái sử dụng bài 1–2.
 * Ví dụ bốn cặp (1,10),(2,20),(3,30),(4,40) -> phát score 28.
 */
void Ai_init ( AI_t *ai){
    if(ai == NULL  ) return;
    ai->count = 0;
}
void AI_Step(void *ctx)
{
    /* TODO BÀI 4. */
    AI_t *ai = (AI_t *)ctx;
    if( ctx == NULL || ai->read == NULL || ai->emit == NULL || ai->count > 3 ) return;
    int8_t frame[2];
    int32_t bias = 0;
    bool received = ai->read(ai->io_ctx, frame);
    if( received == false ) return;
    else {
        uint8_t index = (uint8_t)(ai->count * 2U);
        ai->window[index] =frame[0];
        ai->window[index + 1U] = frame[1];
        ai->count++; 
    }
    if( ai->count == 4 ){
     int8_t conv_output[2];
        int8_t score;

        uint8_t output_count = Conv2Ch(
            ai->window,
            4U,
            AI_KERNEL,
            0,
            conv_output,
            2U
        );
        if (output_count == 2U && GlobalMax(conv_output, output_count, &score)) {
            ai->emit(ai->io_ctx, score);
        }

        ai->count = 0;
    }
}

/* ================= TEST: KHÔNG CẦN SỬA PHẦN DƯỚI ================= */
#define CHECK(expr) do { \
    if (!(expr)) { \
        printf("  Loi tai dong %d: %s\n", __LINE__, #expr); \
        return false; \
    } \
} while (0)

static bool Test_Conv(void)
{
    const int8_t x[8] = {1,10, 2,20, 3,30, 4,40};
    const int8_t neg[8] = {-1,-10, -2,-20, -3,-30, -4,-40};
    const int8_t loud[6] = {127,127, 127,127, 127,127};
    const int8_t big_k[6] = {127,127, 127,127, 127,127};
    int8_t y[3] = {99,99,99};
    CHECK(Conv2Ch(x,4U,AI_KERNEL,0,y,3U) == 2U);
    CHECK(y[0] == 18 && y[1] == 28 && y[2] == 99);
    CHECK(Conv2Ch(x,4U,AI_KERNEL,-20,y,2U) == 2U);
    CHECK(y[0] == 0 && y[1] == 8);
    CHECK(Conv2Ch(neg,4U,AI_KERNEL,0,y,2U) == 2U);
    CHECK(y[0] == 0 && y[1] == 0);
    CHECK(Conv2Ch(loud,3U,big_k,0,y,1U) == 1U && y[0] == 127);
    y[0] = 77; y[1] = 88;
    CHECK(Conv2Ch(x,4U,AI_KERNEL,0,y,1U) == 0U);
    CHECK(y[0] == 77 && y[1] == 88);
    CHECK(Conv2Ch(x,2U,AI_KERNEL,0,y,2U) == 0U);
    CHECK(Conv2Ch(x,17U,AI_KERNEL,0,y,2U) == 0U);
    CHECK(Conv2Ch(NULL,4U,AI_KERNEL,0,y,2U) == 0U);
    CHECK(Conv2Ch(x,4U,NULL,0,y,2U) == 0U);
    CHECK(Conv2Ch(x,4U,AI_KERNEL,0,NULL,2U) == 0U);
    CHECK(y[0] == 77 && y[1] == 88);
    return true;
}

static bool Test_Pool(void)
{
    const int8_t x[4] = {18,28,3,28};
    const int8_t neg[3] = {-8,-3,-9};
    const int8_t one[1] = {-128};
    int8_t out = 66;
    CHECK(GlobalMax(x,4U,&out) && out == 28);
    CHECK(GlobalMax(neg,3U,&out) && out == -3);
    CHECK(GlobalMax(one,1U,&out) && out == -128);
    out = 66;
    CHECK(!GlobalMax(x,0U,&out) && out == 66);
    CHECK(!GlobalMax(x,17U,&out) && out == 66);
    CHECK(!GlobalMax(NULL,4U,&out) && out == 66);
    CHECK(!GlobalMax(x,4U,NULL));
    return true;
}

static void CountCall(void *ctx)
{
    uint32_t *calls = (uint32_t *)ctx;
    if (calls != NULL) { ++(*calls); }
}

static bool Test_Task(void)
{
    uint32_t calls = 0U;
    Task_t t = {100U,10U,CountCall,&calls};
    CHECK(!Task_Run(&t,100U) && calls == 0U && t.last_ms == 100U);
    CHECK(!Task_Run(&t,109U) && calls == 0U && t.last_ms == 100U);
    CHECK(Task_Run(&t,110U) && calls == 1U && t.last_ms == 110U);
    CHECK(!Task_Run(&t,110U) && calls == 1U);
    CHECK(Task_Run(&t,145U) && calls == 2U && t.last_ms == 145U);
    CHECK(!Task_Run(&t,149U) && calls == 2U);
    CHECK(Task_Run(&t,155U) && calls == 3U);
    t.last_ms = UINT32_MAX - 4U;
    CHECK(!Task_Run(&t,4U) && calls == 3U && t.last_ms == UINT32_MAX - 4U);
    CHECK(Task_Run(&t,5U) && calls == 4U && t.last_ms == 5U);
    t.period_ms = 0U;
    CHECK(!Task_Run(&t,100U) && calls == 4U && t.last_ms == 5U);
    t.period_ms = 10U; t.callback = NULL;
    CHECK(!Task_Run(&t,100U) && t.last_ms == 5U);
    CHECK(!Task_Run(NULL,100U));
    t.callback = CountCall; t.ctx = NULL;
    CHECK(Task_Run(&t,100U) && t.last_ms == 100U && calls == 4U);
    return true;
}

typedef struct {
    uint8_t next;
    uint8_t reads;
    uint8_t emitted;
    bool fail_next;
    int8_t scores[2];
} FakeIO_t;

static bool FakeRead(void *ctx, int8_t frame[2])
{
    static const int8_t frames[16] = {
        1,10, 2,20, 3,30, 4,40, 5,50, 6,60, 7,70, 8,80
    };
    FakeIO_t *io = (FakeIO_t *)ctx;
    ++io->reads;
    if (io->fail_next) { io->fail_next = false; return false; }
    if (io->next >= 8U) { return false; }
    frame[0] = frames[2U * io->next];
    frame[1] = frames[2U * io->next + 1U];
    ++io->next;
    return true;
}

static void FakeEmit(void *ctx, int8_t score)
{
    FakeIO_t *io = (FakeIO_t *)ctx;
    if (io->emitted < 2U) { io->scores[io->emitted] = score; }
    ++io->emitted;
}

static bool Test_AI(void)
{
    FakeIO_t io = {0};
    AI_t ai = {{0},0U,FakeRead,FakeEmit,&io};
    Task_t task = {0U,10U,AI_Step,&ai};
    CHECK(!Task_Run(&task,9U) && io.reads == 0U);
    CHECK(Task_Run(&task,10U) && ai.count == 1U && io.reads == 1U);
    CHECK(ai.window[0] == 1 && ai.window[1] == 10 && io.emitted == 0U);
    CHECK(Task_Run(&task,20U) && ai.count == 2U && io.emitted == 0U);
    io.fail_next = true;
    CHECK(Task_Run(&task,30U) && ai.count == 2U && io.reads == 3U);
    CHECK(ai.window[0] == 1 && ai.window[1] == 10);
    CHECK(ai.window[2] == 2 && ai.window[3] == 20);
    CHECK(Task_Run(&task,40U) && ai.count == 3U && io.emitted == 0U);
    CHECK(Task_Run(&task,50U) && ai.count == 0U && io.emitted == 1U);
    CHECK(io.scores[0] == 28 && io.reads == 5U);
    CHECK(Task_Run(&task,60U) && ai.count == 1U);
    CHECK(Task_Run(&task,70U) && ai.count == 2U);
    CHECK(Task_Run(&task,80U) && ai.count == 3U && io.emitted == 1U);
    CHECK(Task_Run(&task,90U) && ai.count == 0U && io.emitted == 2U);
    CHECK(io.scores[1] == 68 && io.reads == 9U);
    CHECK(!Task_Run(&task,95U) && io.reads == 9U);
    CHECK(Task_Run(&task,100U) && ai.count == 0U && io.emitted == 2U);
    CHECK(io.reads == 10U);
    AI_Step(NULL);
    ai.read = NULL; AI_Step(&ai);
    CHECK(io.reads == 10U && ai.count == 0U);
    ai.read = FakeRead; ai.emit = NULL; AI_Step(&ai);
    CHECK(io.reads == 10U && ai.count == 0U);
    ai.emit = FakeEmit; ai.count = 4U; AI_Step(&ai);
    CHECK(io.reads == 10U && ai.count == 4U);
    return true;
}

int main(void)
{
    unsigned passed = 0U;
    bool ok;
    ok = Test_Conv(); passed += ok ? 1U : 0U;
    printf("Bai 1 - Conv hai kenh: %s\n", ok ? "[PASS]" : "[FAIL]");
    ok = Test_Pool(); passed += ok ? 1U : 0U;
    printf("Bai 2 - Global max: %s\n", ok ? "[PASS]" : "[FAIL]");
    ok = Test_Task(); passed += ok ? 1U : 0U;
    printf("Bai 3 - Scheduler: %s\n", ok ? "[PASS]" : "[FAIL]");
    ok = Test_AI(); passed += ok ? 1U : 0U;
    printf("Bai 4 - Pipeline: %s\n", ok ? "[PASS]" : "[FAIL]");
    printf("Tong: %u/4 [PASS]\n", passed);
    return passed == 4U ? 0 : 1;
}
