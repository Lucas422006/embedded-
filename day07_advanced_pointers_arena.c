/**
 * ============================================================================
 * BÀI HỌC & THỬ THÁCH EMBEDDED C - NGÀY 07 (Thời lượng: 60 - 75 phút - Độ khó: Khá/Khó)
 * Chủ đề: Con trỏ Chuyên sâu — Con trỏ cấp 2 (`**`), Tensor Arena Alignment (`uintptr_t`),
 *         Bộ nhớ Chồng lấn (`memmove`), Ma trận Phẳng 2D & Bảng Nhảy Con trỏ Hàm
 * Tài liệu tra cứu chuẩn:
 *   - Con trỏ, uintptr_t & Chuẩn C: https://cppreference.net/c/language.html
 *   - Quy trình lưu code Git:       https://git-scm.com/cheat-sheet
 * ============================================================================
 *
 * ============================================================================
 * PHẦN 1: KIẾN THỨC CHUYÊN SÂU VỀ CON TRỎ (20 PHÚT ĐỌC KỸ BẢN CHẤT BỘ NHỚ)
 * ============================================================================
 *
 * 1. CON TRỎ CẤP 2 (DOUBLE POINTER `T **pp`) — KHI NÀO BẮT BUỘC PHẢI DÙNG?
 * ----------------------------------------------------------------------------
 * Trong C, mọi tham số truyền vào hàm đều là TRUYỀN THAM TRỊ (tạo bản sao).
 *   - Nếu truyền `uint8_t *p`: Bạn sửa được *dữ liệu* tại địa chỉ mà `p` trỏ tới,
 *     nhưng bạn KHÔNG thể thay đổi *địa chỉ* lưu trong biến `p` của hàm gọi!
 *   - Muốn hàm con thay đổi chính biến con trỏ của hàm gọi (ví dụ: đổi trỏ giữa
 *     2 bộ đệm DMA Ping-Pong mà không copy dữ liệu, hoặc đẩy con trỏ đọc luồng
 *     `cursor` tiến lên 3 byte sau khi đọc), bạn BẮT BUỘC truyền địa chỉ của con trỏ
 *     đó vào — tức **Con trỏ cấp 2 (`uint8_t **pp`)**:
 *         *pp = dia_chi_moi;   // Thay đổi thẳng biến con trỏ gốc bên ngoài!
 *
 * 2. CẤP PHÁT TĨNH & CĂN CHỈNH ĐỊA CHỈ (`uintptr_t`) — TRÁI TIM CỦA `tensor_arena`
 * ----------------------------------------------------------------------------
 * Trong Embedded AI (như LiteRT for Microcontrollers), lệnh `malloc()` bị cấm.
 * Thay vào đó, ta xin 1 mảng tĩnh `uint8_t arena[N]` rồi tự cắt từng miếng RAM
 * (Bump Allocator). Khó ở chỗ: Tập lệnh SIMD/DSP của Cortex-M hay ESP32-S3 đòi hỏi
 * địa chỉ bắt đầu của Tensor phải chia hết cho `alignment` (4, 8 hoặc 16 byte)!
 *   - Ta ép địa chỉ con trỏ sang số nguyên không dấu `uintptr_t` (từ `<stdint.h>`).
 *   - Công thức Bitwise làm tròn LÊN tới bội số gần nhất của `align` (với `align` là lũy thừa của 2):
 *         uintptr_t aligned = (addr + (align - 1U)) & ~(align - 1U);
 *   - Mẹo kiểm tra một số `align > 0` có phải lũy thừa của 2 (1, 2, 4, 8, 16...) hay không:
 *         (align > 0U) && ((align & (align - 1U)) == 0U)
 *
 * 3. VÙNG NHỚ CHỒNG LẤN (OVERLAPPING MEMORY) — BẪY CHẾT NGƯỜI KHI DỊCH MẢNG
 * ----------------------------------------------------------------------------
 * Giả sử bạn có mảng Sliding Window `a = [10, 20, 30, 40, 50, 60]` và muốn chép
 * 4 phần tử đầu (`src = &a[0]`) dịch sang phải 2 ô (`dst = &a[2]`):
 *   - Nếu bạn copy XUÔI từ trái sang phải (`i = 0 -> 3`):
 *     `a[2] = a[0]` (ghi đè `10` lên ô chứa `30`!) -> đến bước `i = 2`, giá trị `30`
 *     đã bị phá hủy thành `10`! Đây là lý do `memcpy()` bị lỗi trên vùng chồng lấn.
 *   - Giải pháp chuẩn của `memmove()`: Nếu `(dst > src) && (dst < src + n)` thì
 *     BẮT BUỘC phải copy NGƯỢC từ cuối về đầu (`i = n - 1` giảm dần về `0`).
 *
 * 4. TRUY CẬP MA TRẬN PHẲNG 2D (ROW-MAJOR) BẰNG CON TRỎ 1 CHIỀU
 * ----------------------------------------------------------------------------
 * Trọng số của một lớp mạng nơ-ron `Dense` có `out_len` nơ-ron và `in_len` đầu vào
 * được trải phẳng thành mảng 1 chiều `const int8_t *weights` kích thước `out_len * in_len`.
 *   - Hàng thứ `r` bắt đầu tại địa chỉ: `const int8_t *row_ptr = weights + (r * in_len);`
 *   - Phần tử hàng `r`, cột `c` là:     `*(row_ptr + c)`
 *
 * 5. BẢNG NHẢY CON TRỎ HÀM (DISPATCH TABLE) — XÓA SỔ `switch-case` DÀI DÒNG
 * ----------------------------------------------------------------------------
 * Thay vì viết 10 nhánh `switch (opcode)`, kỹ sư nhúng khai báo một mảng hằng số
 * trong Flash chứa các con trỏ hàm:
 *     static const OpHandler_t OP_TABLE[4] = { Op_Add, Op_Sub, Op_Mul, Op_Max };
 * Khi nhận `opcode` hợp lệ (`opcode < 4`), CPU nhảy thẳng tới hàm cần chạy trong O(1):
 *     int32_t res = OP_TABLE[opcode](a, b);
 * ============================================================================
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* ============================================================================
 * PHẦN 2: 5 THỬ THÁCH CHUYÊN SÂU NGÀY 07 (TỰ SUY NGHĨ — KHÔNG CÓ CODE GỢI Ý)
 * ============================================================================ */

/* ----------------------------------------------------------------------------
 * BÀI TẬP 1: ZERO-COPY DOUBLE BUFFERING (PING-PONG DMA) BẰNG CON TRỎ CẤP 2
 * Đặc tả kỹ thuật:
 *   - Trong hệ thống thu mẫu AI liên tục, DMA ghi vào bộ đệm `*rx_buf` trong khi
 *     CPU suy luận trên `*proc_buf`. Khi DMA báo đầy, ta đổi chỗ 2 con trỏ này
 *     cho nhau ngay lập tức trong O(1) mà KHÔNG copy bất kỳ byte dữ liệu nào.
 *   - Kiểm tra an toàn: Nếu `rx_buf == NULL`, `proc_buf == NULL`, `*rx_buf == NULL`
 *     hoặc `*proc_buf == NULL` -> trả về `false`.
 *   - Thực hiện hoán đổi địa chỉ mà `*rx_buf` và `*proc_buf` đang trỏ tới, trả về `true`.
 * ---------------------------------------------------------------------------- */
bool DMA_SwapBuffers(uint8_t **rx_buf, uint8_t **proc_buf) {
    // TODO 1: Viết logic kiểm tra NULL và hoán đổi 2 con trỏ cấp 1 thông qua con trỏ cấp 2
    if( rx_buf == NULL || proc_buf == NULL || *rx_buf == NULL || *proc_buf == NULL  ) return false;

    uint8_t *ptr = *rx_buf;
    *rx_buf = *proc_buf;
    *proc_buf = ptr;
    return true;
}


/* ----------------------------------------------------------------------------
 * BÀI TẬP 2: TỰ XÂY DỰNG BỘ CẤP PHÁT TĨNH `tensor_arena` CÓ CĂN CHỈNH ĐỊA CHỈ
 * Đặc tả kỹ thuật:
 *   - Cấu trúc quản lý vùng RAM tĩnh `TensorArena_t` được định nghĩa bên dưới.
 *   - Yêu cầu hàm `Arena_AllocAligned(arena, size, alignment)`:
 *       1. Kiểm tra tính hợp lệ: `arena != NULL`, `arena->base != NULL`, `size > 0`,
 *          và `alignment` PHẢI là lũy thừa của 2 (1, 2, 4, 8, 16, 32...). Nếu sai -> trả về `NULL`.
 *       2. Tính địa chỉ tuyệt đối hiện tại: `curr_addr = (uintptr_t)(arena->base + arena->offset)`.
 *       3. Làm tròn LÊN `curr_addr` tới địa chỉ `aligned_addr` chia hết cho `alignment`.
 *       4. Tính lượng đệm phát sinh `padding = (size_t)(aligned_addr - curr_addr)`.
 *       5. Kiểm tra chống tràn RAM (chú ý cả trường hợp `padding + size` vượt `capacity - offset`):
 *          Nếu `arena->offset + padding + size > arena->capacity` -> GIỮ NGUYÊN `arena->offset`
 *          và trả về `NULL`.
 *       6. Cập nhật `arena->offset += padding + size;` và trả về `(void *)aligned_addr`.
 * ---------------------------------------------------------------------------- */
typedef struct {
    uint8_t *base;      // Địa chỉ đầu của mảng RAM tĩnh
    size_t   capacity;  // Tổng dung lượng tối đa (bytes)
    size_t   offset;    // Số byte đã sử dụng tính từ `base`
} TensorArena_t;
bool test ( size_t x){
    if( x == 0 ) return false;
    if( x == 1 ) return true;
    while( 1){
        if ( x % 2 == 0 ) {
            x /= 2;
        } else {
            return false;
        }
        if( x == 1 ) return true;
    } 
    
}

void *Arena_AllocAligned(TensorArena_t *arena, size_t size, size_t alignment) {
    // TODO 2: Tự viết bộ cấp phát tĩnh có căn chỉnh địa chỉ bằng uintptr_t
    if( arena == NULL || arena->base == NULL || size ==  0 ) return NULL;
    if( test( alignment) == false ) return NULL;
    uintptr_t curr_addr = (uintptr_t)( arena->base + arena->offset);
    uintptr_t aligned = (curr_addr + (alignment - 1U)) & ~(uintptr_t)(alignment - 1U);
    size_t padding = (size_t)(aligned - curr_addr);
    if( (arena->offset + padding + size ) > arena->capacity) return NULL;
    else {
        arena->offset += padding + size;
        return (void *)aligned;
    }
    
}


/* ----------------------------------------------------------------------------
 * BÀI TẬP 3: TỰ VIẾT HÀM `Mem_SafeCopy` (TƯƠNG ĐƯƠNG `memmove`) BẰNG CON TRỎ `void *`
 * Đặc tả kỹ thuật:
 *   - Sao chép chính xác `n` byte từ `src` sang `dst`, hoạt động AN TOÀN TUYỆT ĐỐI
 *     ngay cả khi vùng nhớ nguồn và đích bị chồng lấn (dịch trái hoặc dịch phải).
 *   - Nếu `dst == NULL` HOẶC `src == NULL`: trả về `NULL`.
 *   - Nếu `n == 0` HOẶC `dst == src`: không cần copy, trả về `dst`.
 *   - Nếu vùng đích nằm chồng lên phía sau vùng nguồn (`d > s && d < s + n`):
 *     Sao chép NGƯỢC từ cuối về đầu. Ngược lại: sao chép XUÔI từ đầu về cuối.
 *   - Không được gọi hàm thư viện chuẩn `memcpy` hay `memmove`! Trả về `dst`.
 * ---------------------------------------------------------------------------- */
void *Mem_SafeCopy(void *dst, const void *src, size_t n) {
    // TODO 3: Xử lý copy xuôi/ngược theo tương quan địa chỉ con trỏ
    if( dst == NULL || src == NULL ) return NULL;
    if( n == 0 || dst == src ) return dst;
    uint8_t *d = (uint8_t *)dst;
    const uint8_t *s = (const uint8_t *)src;
    if( d > s && d < s + n) {
        for( int i = n - 1; i >= 0; i--){
            d[i] = s[i];
        }
        return d;
    } else {
        for( int i = 0; i < n; i++){
            d[i] = s[i];
        }
        return d;
    }
    return NULL;
}


/* ----------------------------------------------------------------------------
 * BÀI TẬP 4: NHÂN MA TRẬN PHẲNG 2D CHO LỚP MẠNG NƠ-RON TINYML (`Dense INT8`)
 * Đặc tả kỹ thuật:
 *   - Tính đầu ra của một lớp Fully Connected (Dense) gồm `out_len` nơ-ron từ vector
 *     đầu vào `input` (độ dài `in_len`):
 *       + `weights`: Ma trận 2D kích thước `[out_len][in_len]` trải phẳng theo hàng.
 *       + `bias`:    Mảng `out_len` hệ số điều chỉnh kiểu `int32_t`.
 *       + Với mỗi nơ-ron `r` từ `0` đến `out_len - 1`:
 *           1. Khởi tạo `int32_t acc = bias[r];`
 *           2. Lấy con trỏ đầu hàng: `const int8_t *w_row = weights + (r * in_len);`
 *           3. Cộng dồn tích vô hướng: `acc += (int32_t)w_row[c] * (int32_t)input[c]`
 *              (khuyến khích duyệt bằng con trỏ).
 *           4. Giảm tỷ lệ lượng tử hóa bằng dịch phải số học: `acc >>= right_shift;`
 *           5. Kẹp biên bão hòa (Clamp) `acc` vào miền `[-128, 127]` và ghi vào `output[r]`.
 *   - Nếu bất kỳ con trỏ nào bằng `NULL`, hoặc `in_len == 0`, hoặc `out_len == 0`: trả về `false`.
 *   - Hoàn thành hợp lệ: trả về `true`.
 * ---------------------------------------------------------------------------- */
bool TinyML_DenseLayer_INT8(const int8_t *input,
                            size_t in_len,
                            const int8_t *weights,
                            const int32_t *bias,
                            int8_t *output,
                            size_t out_len,
                            uint8_t right_shift) {
    // TODO 4: Triển khai lớp Dense INT8 bằng truy cập ma trận phẳng qua con trỏ
    if( in_len == 0 || out_len == 0  || input == NULL || output == NULL || bias == NULL || weights == NULL ) return false;
    
    int r = 0;
    while( r < out_len ){
        int  n = 0;
        int32_t acc = bias[r];
        const int8_t *w_row = weights + ( r* in_len);
    while ( n < in_len){
        acc += (int32_t)w_row[n] * ( int32_t)input[n];
        
        n++;
    }
    acc >>= right_shift;
    if( acc >= 127 ) acc = 127;
    else if( acc < -128 ) acc = -128;
    output[r] = (int8_t)acc;
    r++;
}

    return true;
}


/* ----------------------------------------------------------------------------
 * BÀI TẬP 5: TRÌNH THÔNG DỊCH LUỒNG BYTE (CON TRỎ CẤP 2 + BẢNG NHẢY CON TRỎ HÀM)
 * Đặc tả kỹ thuật:
 *   - Một luồng byte lệnh chứa nhiều gói liên tiếp, mỗi gói dài ĐÚNG 3 byte:
 *         Byte 0: `opcode` (uint8_t): 0 = ADD, 1 = SUB, 2 = MUL, 3 = MAX
 *         Byte 1: `arg_a`  (int8_t có dấu)
 *         Byte 2: `arg_b`  (int8_t có dấu)
 *   - Yêu cầu bắt buộc:
 *       1. Tự viết 4 hàm `static int32_t` cho 4 phép toán (ADD, SUB, MUL, MAX)
 *          và khai báo bảng nhảy `static const OpHandler_t OP_TABLE[4] = { ... };`.
 *       2. KHÔNG ĐƯỢC dùng `switch-case` hay `if-else` để rẽ nhánh theo `opcode`!
 *       3. Trong hàm `VM_ExecuteNextCmd(const uint8_t **cursor, const uint8_t *end, int32_t *out_res)`:
 *          - Kiểm tra an toàn: `cursor != NULL`, `*cursor != NULL`, `end != NULL`, `out_res != NULL`.
 *          - Dùng phép trừ con trỏ kiểm tra xem từ `*cursor` đến `end` còn đủ 3 byte không
 *            (`*cursor > end` hoặc `(size_t)(end - *cursor) < 3U` -> trả về `false`, KHÔNG dịch `*cursor`).
 *          - Đọc `opcode = (*cursor)[0]`. Nếu `opcode >= 4U` -> trả về `false`, KHÔNG dịch `*cursor`.
 *          - Đọc `arg_a = (int8_t)(*cursor)[1]`, `arg_b = (int8_t)(*cursor)[2]`.
 *          - Gọi `*out_res = OP_TABLE[opcode](arg_a, arg_b);`.
 *          - Đẩy con trỏ đọc của bên gọi tiến lên 3 byte (`*cursor += 3U;`) và trả về `true`.
 * ---------------------------------------------------------------------------- */
typedef int32_t (*OpHandler_t)(int32_t a, int32_t b);

/* TODO 5A: Viết 4 hàm static (ADD, SUB, MUL, MAX) và mảng bảng nhảy `OP_TABLE[4]` tại đây */
static int32_t ADD (int32_t a, int32_t b ){
    return a + b;
}
static int32_t SUB( int32_t a, int32_t b){
    return a - b;
}
static int32_t MUL( int32_t a, int32_t b){
    return a*b;
} 
static int32_t MAX( int32_t a, int32_t b){
    if( a > b ) return a;
    else return b;
}
static const OpHandler_t OP_TABLE[4] = { ADD , SUB, MUL, MAX };

bool VM_ExecuteNextCmd(const uint8_t **cursor, const uint8_t *end, int32_t *out_res) {
    // TODO 5B: Kiểm tra biên bằng phép trừ con trỏ, gọi OP_TABLE[opcode] và đẩy *cursor += 3
    if( out_res == NULL || end == NULL || cursor == NULL || *cursor == NULL ) return false;
    if( *cursor > end || (size_t)( end - *cursor ) < 3U) return false;
    int32_t opcode = *cursor[0]; 
    if( opcode >= 4U) return false;
    int32_t arg_a = (int8_t)(*cursor)[1];
    int32_t arg_b = (int8_t)(*cursor)[2];
    *out_res = OP_TABLE[opcode](arg_a, arg_b);
    *cursor += 3U;
    return true;
}


/* ============================================================================
 * PHẦN 3: HỆ THỐNG CHẤM ĐIỂM TỰ ĐỘNG NGÀY 07 (5 BÀI TẬP - KHÔNG SỬA HÀM MAIN)
 * ============================================================================ */
int main(void) {
    uint8_t pass_count = 0;

    printf("=== BAT DAU CHAM DIEM THU THACH CHUYEN SAU NGAY 07 (5 BAI TAP) ===\n\n");

    /* --- Test Bài tập 1: Zero-copy Double Buffering --- */
    uint8_t ping_buf[4] = {1, 2, 3, 4};
    uint8_t pong_buf[4] = {9, 8, 7, 6};
    uint8_t *p_rx   = ping_buf;
    uint8_t *p_proc = pong_buf;
    uint8_t *p_null = NULL;

    bool sw_ok   = DMA_SwapBuffers(&p_rx, &p_proc);
    bool sw_bad1 = DMA_SwapBuffers(NULL, &p_proc);
    bool sw_bad2 = DMA_SwapBuffers(&p_null, &p_proc);

    if (sw_ok && !sw_bad1 && !sw_bad2 && (p_rx == pong_buf) && (p_proc == ping_buf)) {
        printf("[PASS] Bai tap 1: DMA_SwapBuffers hoan doi con tro cap 2 O(1) chinh xac!\n");
        pass_count++;
    } else {
        printf("[FAIL] Bai tap 1: sw_ok=%d, p_rx==pong(%d), p_proc==ping(%d)\n",
               sw_ok, (p_rx == pong_buf), (p_proc == ping_buf));
    }

    /* --- Test Bài tập 2: Tensor Arena Aligned Allocator --- */
    uint8_t raw_arena[64] = {0};
    TensorArena_t arena = { .base = raw_arena, .capacity = sizeof(raw_arena), .offset = 0U };

    void *a1 = Arena_AllocAligned(&arena, 3U, 1U);
    void *a2 = Arena_AllocAligned(&arena, 10U, 16U);
    size_t offset_after_a2 = arena.offset;
    void *a_bad_align = Arena_AllocAligned(&arena, 4U, 6U);   // 6 không phải lũy thừa của 2
    void *a_oom       = Arena_AllocAligned(&arena, 100U, 4U); // Vượt quá 64 bytes

    bool a2_aligned = (a2 != NULL) && ((((uintptr_t)a2) & 15U) == 0U) &&
                      ((uintptr_t)a2 >= ((uintptr_t)a1 + 3U));

    if ((a1 != NULL) && a2_aligned && (a_bad_align == NULL) && (a_oom == NULL) &&
        (arena.offset == offset_after_a2)) {
        printf("[PASS] Bai tap 2: Arena_AllocAligned can chinh 16-byte & bao ve tran RAM chinh xac!\n");
        pass_count++;
    } else {
        printf("[FAIL] Bai tap 2: a1=%p, a2=%p (mod16=%u), bad_align=%p, oom=%p\n",
               a1, a2, (unsigned int)(((uintptr_t)a2) & 15U), a_bad_align, a_oom);
    }

    /* --- Test Bài tập 3: Mem_SafeCopy (Overlapping memmove) --- */
    uint8_t win[8] = {10, 20, 30, 40, 50, 60, 70, 80};
    // Dịch PHẢI chồng lấn: copy 5 byte [0..4] sang [2..6] -> {10, 20, 10, 20, 30, 40, 50, 80}
    void *r_right = Mem_SafeCopy(&win[2], &win[0], 5U);
    bool right_ok = (r_right == &win[2]) &&
                    (win[0] == 10) && (win[1] == 20) && (win[2] == 10) && (win[3] == 20) &&
                    (win[4] == 30) && (win[5] == 40) && (win[6] == 50) && (win[7] == 80);

    // Dịch TRÁI chồng lấn: copy 5 byte [2..6] sang [0..4] -> {10, 20, 30, 40, 50, 40, 50, 80}
    void *r_left = Mem_SafeCopy(&win[0], &win[2], 5U);
    bool left_ok = (r_left == &win[0]) &&
                   (win[0] == 10) && (win[1] == 20) && (win[2] == 30) && (win[3] == 40) &&
                   (win[4] == 50) && (win[5] == 40) && (win[6] == 50) && (win[7] == 80);

    if (right_ok && left_ok && (Mem_SafeCopy(NULL, win, 4U) == NULL)) {
        printf("[PASS] Bai tap 3: Mem_SafeCopy xu ly vung nho chong lan trai & phai chinh xac!\n");
        pass_count++;
    } else {
        printf("[FAIL] Bai tap 3: right_ok=%d, left_ok=%d, win=[%u %u %u %u %u %u %u %u]\n",
               right_ok, left_ok, win[0], win[1], win[2], win[3], win[4], win[5], win[6], win[7]);
    }

    /* --- Test Bài tập 4: TinyML Dense Layer (Ma trận phẳng 2D) --- */
    const int8_t  x_in[3] = {10, -20, 30};
    const int8_t  W[6]    = {
         2, -1,   4,   // Hàng 0: 2*10 + (-1)*(-20) + 4*30 = 160; +20 = 180; >>2 = 45
        -5,  2, -10    // Hàng 1: -50 - 40 - 300 = -390; -210 = -600; >>2 = -150 -> clamp -128
    };
    const int32_t b[2]    = {20, -210};
    int8_t y_out[2]       = {0, 0};

    bool dense_ok  = TinyML_DenseLayer_INT8(x_in, 3U, W, b, y_out, 2U, 2U);
    bool dense_bad = TinyML_DenseLayer_INT8(NULL, 3U, W, b, y_out, 2U, 2U);

    if (dense_ok && !dense_bad && (y_out[0] == 45) && (y_out[1] == -128)) {
        printf("[PASS] Bai tap 4: TinyML_DenseLayer_INT8 tinh ma tran 2D phang (y=[%d, %d]) chinh xac!\n",
               y_out[0], y_out[1]);
        pass_count++;
    } else {
        printf("[FAIL] Bai tap 4: dense_ok=%d, y_out=[%d, %d] (ky vong [45, -128])\n",
               dense_ok, y_out[0], y_out[1]);
    }

    /* --- Test Bài tập 5: Trình thông dịch luồng byte (Double Pointer + Dispatch Table) --- */
    const uint8_t cmd_stream[11] = {
        0U, (uint8_t)(-10), 25U,              // Lệnh 1: ADD(-10, 25) = 15
        2U, (uint8_t)(-4),  8U,               // Lệnh 2: MUL(-4, 8)   = -32
        3U, (uint8_t)(-5),  (uint8_t)(-2),    // Lệnh 3: MAX(-5, -2)  = -2
        1U, 5U                                // Rác cuối luồng (chỉ còn 2 byte < 3 -> phải từ chối!)
    };
    const uint8_t *cur = cmd_stream;
    const uint8_t *end = cmd_stream + sizeof(cmd_stream);
    int32_t res1 = 0, res2 = 0, res3 = 0, res4 = 0;

    bool e1 = VM_ExecuteNextCmd(&cur, end, &res1);
    bool e2 = VM_ExecuteNextCmd(&cur, end, &res2);
    bool e3 = VM_ExecuteNextCmd(&cur, end, &res3);
    const uint8_t *cur_before_fail = cur;
    bool e4 = VM_ExecuteNextCmd(&cur, end, &res4); // Phải trả false vì chỉ còn 2 byte

    const uint8_t bad_opcode[3] = {9U, 1U, 2U};
    const uint8_t *cur_bad = bad_opcode;
    bool e_bad = VM_ExecuteNextCmd(&cur_bad, bad_opcode + 3U, &res4);

    if (e1 && e2 && e3 && !e4 && !e_bad &&
        (res1 == 15) && (res2 == -32) && (res3 == -2) &&
        (cur == cur_before_fail) && (cur == cmd_stream + 9U) && (cur_bad == bad_opcode)) {
        printf("[PASS] Bai tap 5: VM_ExecuteNextCmd (con tro cap 2 + bang nhay) chinh xac!\n");
        pass_count++;
    } else {
        printf("[FAIL] Bai tap 5: e1..4=[%d %d %d %d], res=[%d, %d, %d] (ky vong 15, -32, -2), offset=%td\n",
               e1, e2, e3, e4, res1, res2, res3, (cur - cmd_stream));
    }

    printf("\n=== KET QUA TONG KET NGAY 07: %u / 5 BAI TAP DAT [PASS] ===\n", pass_count);
    return 0;
}