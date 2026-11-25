/**
 * ============================================================================
 * BÀI TẬP ÔN TẬP EMBEDDED C - NGÀY 09 (Thời lượng: 60 phút - Độ khó: Trung bình)
 * Chủ đề: Ôn tập Bộ Đệm Vòng (Ring Buffer) & Máy Trạng Thái (FSM) mức vừa sức
 * Tài liệu tra cứu chuẩn:
 *   - Chuẩn ngôn ngữ C:       https://cppreference.net/c/language.html
 *   - Quy trình lưu code Git: https://git-scm.com/cheat-sheet
 * ============================================================================
 * QUY TẮC NGÀY 09:
 * - Không có gợi ý công thức hay hướng dẫn từng bước.
 * - Độ khó được điều chỉnh về mức TRUNG BÌNH (4 bài gọn) để hoàn thành trọn vẹn
 *   trong 1 giờ, củng cố chắc chắn Ring Buffer và FSM trước khi sang chủ đề mới.
 * - Hoàn thành cả 4 bài đạt 4 / 4 [PASS] rồi đẩy lên GitHub.
 * ============================================================================
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* ============================================================================
 * BÀI TẬP 1: BỘ ĐỆM VÒNG FIFO CỐT LÕI (CORE RING BUFFER)
 * Yêu cầu hành vi:
 *   - Tự thiết kế các trường bên trong `SimpleRB_t` (xóa biến `_replace_me`).
 *   - Hoạt động trên mảng tĩnh `buf` do bên ngoài cung cấp, sử dụng đủ `capacity`
 *     ô nhớ (không lãng phí ô trống).
 *   - `SRB_Init`:  Khởi tạo bộ đệm. Trả `false` nếu tham số không hợp lệ.
 *   - `SRB_Push`:  Ghi 1 byte vào bộ đệm. Trả `false` nếu đầy hoặc tham số sai.
 *   - `SRB_Pop`:   Lấy 1 byte cũ nhất ra `*out_val`. Trả `false` nếu rỗng hoặc sai.
 *   - `SRB_Count`: Trả về số byte hiện đang nằm trong bộ đệm (trả 0 nếu `rb == NULL`).
 * ============================================================================ */
typedef struct {
    /* TODO 1A: Xóa biến tạm và tự thiết kế các trường quản lý bộ đệm vòng */
    uint8_t *buf;
    size_t capacity;
    size_t head;
    size_t tail;
    size_t count;
} SimpleRB_t;

bool SRB_Init(SimpleRB_t *rb, uint8_t *buf, size_t capacity) {
    // TODO 1B
    if( rb == NULL || buf == NULL  || capacity == 0) return false;
    rb->buf = buf;
    rb->capacity =capacity;
    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;
    return true;
}

bool SRB_Push(SimpleRB_t *rb, uint8_t val) {
    // TODO 1C
    if( rb == NULL || rb->buf == NULL|| rb->capacity == 0 || rb->count == rb->capacity ) return false;
    rb->buf[rb->head] = val;
    rb->head++;
    if( rb->head == rb->capacity) rb->head = 0;
    rb->count++;
    return true;
}

bool SRB_Pop(SimpleRB_t *rb, uint8_t *out_val) {
    // TODO 1D
    if( rb == NULL || rb->buf == NULL || out_val == NULL || rb->count == 0 ) return false;
    *out_val = rb->buf[rb->tail] ;
    rb->tail++;
    if( rb->tail == rb->capacity ) rb->tail = 0;
    rb->count--;
    return true;
}

size_t SRB_Count(const SimpleRB_t *rb) {
    // TODO 1E
    if( rb == NULL || rb->buf == NULL || rb->capacity == 0 ) return 0U;
    return rb->count;
}


/* ============================================================================
 * BÀI TẬP 2: GHI VÀ ĐỌC NHIỀU BYTE LIÊN TIẾP (MULTI-BYTE PUSH / POP)
 * Yêu cầu hành vi:
 *   - `SRB_PushArray`: Đẩy tối đa `len` byte từ mảng `src` vào `rb`. Nếu bộ đệm
 *     đầy giữa chừng thì dừng lại và trả về số byte thực sự đã ghi được.
 *   - `SRB_PopArray`:  Rút tối đa `len` byte từ `rb` sang mảng `dst`. Nếu bộ đệm
 *     rỗng giữa chừng thì dừng lại và trả về số byte thực sự đã rút được.
 *   - Xử lý an toàn khi con trỏ `NULL` hoặc `len == 0` (trả về 0).
 * ============================================================================ */
size_t SRB_PushArray(SimpleRB_t *rb, const uint8_t *src, size_t len) {
    // TODO 2A
    if( rb == NULL || rb->buf == NULL || src == NULL || rb->capacity == 0 ) return 0U;
    size_t count = 0;
    for( size_t i = 0; i < len; i++){
        if( SRB_Push(rb,src[i]) == false) break;
        else count++;
    }
    return count;
}

size_t SRB_PopArray(SimpleRB_t *rb, uint8_t *dst, size_t len) {
    // TODO 2B
    if( rb == NULL || rb->buf == NULL || dst == NULL || rb->capacity == 0 ) return 0U;
    size_t count = 0;
    for( size_t i = 0; i < len; i++){
        if( SRB_Pop( rb, &dst[i]) == false) break;
        else count++;
    }
    return count;
}


/* ============================================================================
 * BÀI TẬP 3: MÁY TRẠNG THÁI GIẢI MÃ GÓI TIN CỐ ĐỊNH 4 BYTE (FIXED-FRAME FSM)
 * Đặc tả khung truyền cố định 4 byte:
 *   - Byte 0 (Header): Luôn là `0xAA`
 *   - Byte 1 (Cmd):    Mã lệnh (giá trị bất kỳ `0x00 .. 0xFF`)
 *   - Byte 2 (Data):   Dữ liệu cảm biến (giá trị bất kỳ `0x00 .. 0xFF`)
 *   - Byte 3 (Footer): Luôn là `0x55`
 *
 * Yêu cầu hành vi:
 *   - Tự thiết kế cấu trúc `FixedFSM_t` để lưu trạng thái giữa các lần gọi.
 *   - `FSM_Init`: Đưa máy trạng thái về trạng thái chờ Header ban đầu.
 *   - `FSM_Feed`: Nạp từng byte một.
 *       + Chỉ trả `true` và xuất `cmd`, `data` vào `out_pkt` đúng tại byte
 *         thứ 4 nếu Footer bằng `0x55`, sau đó quay về chờ gói mới.
 *       + Nếu đang chờ Header mà gặp byte khác `0xAA`: bỏ qua, trả `false`.
 *       + Nếu ở byte thứ 4 mà Footer KHÁC `0x55`: hủy gói lỗi, trả `false`
 *         và tự phục hồi để bắt được gói hợp lệ kế tiếp.
 *         (Chú ý tình huống biên: Nếu byte Footer bị sai đó tình cờ lại chính
 *         là `0xAA`, hãy cân nhắc xem nó có thể là Header của gói mới không!)
 * ============================================================================ */
#define PKT_HEADER_BYTE  (0xAAU)
#define PKT_FOOTER_BYTE  (0x55U)

typedef struct {
    uint8_t cmd;
    uint8_t data;
} Packet4B_t;
typedef enum{
    Header = 0,
    Cmd,
    Data,
    Footer
}FixedState_t;
typedef struct {
    /* TODO 3A: Xóa biến tạm và tự thiết kế các trường trạng thái của FSM */
    FixedState_t state;
    uint8_t Cmd;
    uint8_t Data;

} FixedFSM_t;

void FSM_Init(FixedFSM_t *fsm) {
    // TODO 3B
    if( fsm == NULL) return;
    fsm->state = Header;

}

bool FSM_Feed(FixedFSM_t *fsm, uint8_t byte, Packet4B_t *out_pkt) {
    // TODO 3C
    if( fsm == NULL || out_pkt == NULL ) return false;
    switch (fsm->state)
    {
    case Header:
        /* code */
        if( byte != PKT_HEADER_BYTE ){
            return false;
        } else {
            fsm->state = Cmd;
            return false;
        }
        break;
    case Cmd:
        fsm->Cmd = byte;
        fsm->state = Data;
        return false;
        break;
    case Data:
        fsm->Data = byte;
        fsm->state = Footer;
        return false;
        break;
    case Footer:
        if( byte == PKT_FOOTER_BYTE){
            out_pkt->cmd = fsm->Cmd;
            out_pkt->data = fsm->Data;
            fsm->state = Header;
            return true;
        } else {
            if( byte == PKT_HEADER_BYTE ){
                fsm->state = Cmd;
                return false;
            }
            fsm->state = Header;
            return false;
        }
        break;
    default:
        break;
    }
    return false;
}


/* ============================================================================
 * BÀI TẬP 4: GHÉP NỐI RING BUFFER + FSM (XỬ LÝ LUỒNG VÀ TỔNG HỢP KẾT QUẢ)
 * Yêu cầu hành vi:
 *   - `Stream_DrainAndParse`: Rút lần lượt toàn bộ byte đang có trong `rb`
 *     và nạp qua `fsm`.
 *   - Mỗi khi giải mã thành công 1 gói hợp lệ:
 *       + Tăng bộ đếm số gói hợp lệ.
 *       + Cập nhật `*out_last_pkt` bằng gói hợp lệ vừa giải mã (nếu `out_last_pkt != NULL`).
 *   - Trả về tổng số gói hợp lệ giải mã được trong lần xử lý này.
 *   - Nếu `rb == NULL` hoặc `fsm == NULL`, trả về 0.
 * ============================================================================ */
uint32_t Stream_DrainAndParse(SimpleRB_t *rb, FixedFSM_t *fsm, Packet4B_t *out_last_pkt) {
    // TODO 4
    if( rb == NULL || fsm == NULL|| rb->buf == NULL ) return 0U;
    uint8_t byte;
    Packet4B_t pkt;
    uint32_t count = 0;
    while(SRB_Pop(rb,&byte)){
        if( FSM_Feed(fsm,byte,&pkt) == true){
            count++;
            if(out_last_pkt != NULL){
                *out_last_pkt = pkt;
            }
        }
    }

    return count;
}


/* ============================================================================
 * HỆ THỐNG CHẤM ĐIỂM TỰ ĐỘNG NGÀY 09 (4 BÀI TẬP - KHÔNG SỬA HÀM MAIN)
 * ============================================================================ */
int main(void) {
    uint8_t pass_count = 0U;

    printf("=== BAT DAU CHAM DIEM ON TAP NGAY 09 (MUC TRUNG BINH) ===\n\n");

    /* --- Test Bài tập 1: Core Ring Buffer FIFO --- */
    uint8_t mem1[4] = {0};
    SimpleRB_t rb1;
    bool t1 = SRB_Init(&rb1, mem1, 4U) && !SRB_Init(&rb1, NULL, 4U) && !SRB_Init(&rb1, mem1, 0U);
    t1 = t1 && SRB_Push(&rb1, 11U) && SRB_Push(&rb1, 22U) && SRB_Push(&rb1, 33U) && SRB_Push(&rb1, 44U);
    t1 = t1 && !SRB_Push(&rb1, 55U) && (SRB_Count(&rb1) == 4U);

    uint8_t v = 0U;
    t1 = t1 && SRB_Pop(&rb1, &v) && (v == 11U);
    t1 = t1 && SRB_Push(&rb1, 55U) && (SRB_Count(&rb1) == 4U); // Quay vòng về ô đầu

    uint8_t expected1[4] = {22U, 33U, 44U, 55U};
    for (uint8_t i = 0U; i < 4U; i++) {
        t1 = t1 && SRB_Pop(&rb1, &v) && (v == expected1[i]);
    }
    t1 = t1 && !SRB_Pop(&rb1, &v) && (SRB_Count(&rb1) == 0U);

    if (t1) {
        printf("[PASS] Bai tap 1: SimpleRB FIFO ghi/doc va quay vong chinh xac!\n");
        pass_count++;
    } else {
        printf("[FAIL] Bai tap 1: Kiem tra lai logic Push/Pop/Count va quay vong (count=%zu)\n", SRB_Count(&rb1));
    }

    /* --- Test Bài tập 2: Multi-byte Push / Pop --- */
    uint8_t mem2[5] = {0};
    SimpleRB_t rb2;
    SRB_Init(&rb2, mem2, 5U);

    const uint8_t src_a[3] = {1U, 2U, 3U};
    const uint8_t src_b[4] = {4U, 5U, 6U, 7U};
    uint8_t dst_a[2] = {0};
    uint8_t dst_b[6] = {0};

    size_t w1 = SRB_PushArray(&rb2, src_a, 3U); // Ghi 3 byte: [1, 2, 3]
    size_t r1 = SRB_PopArray(&rb2, dst_a, 2U);  // Đọc 2 byte: [1, 2], còn [3]
    size_t w2 = SRB_PushArray(&rb2, src_b, 4U); // Còn 4 chỗ trống -> ghi đủ 4 byte: [3, 4, 5, 6, 7]
    size_t w3 = SRB_PushArray(&rb2, src_a, 3U); // Đã đầy -> ghi được 0 byte
    size_t r2 = SRB_PopArray(&rb2, dst_b, 6U);  // Chỉ có 5 byte -> đọc đúng 5 byte

    bool seq2 = (dst_a[0] == 1U) && (dst_a[1] == 2U) &&
                (dst_b[0] == 3U) && (dst_b[1] == 4U) && (dst_b[2] == 5U) &&
                (dst_b[3] == 6U) && (dst_b[4] == 7U);
    if ((w1 == 3U) && (r1 == 2U) && (w2 == 4U) && (w3 == 0U) && (r2 == 5U) && seq2) {
        printf("[PASS] Bai tap 2: SRB_PushArray / SRB_PopArray hoat dong chinh xac!\n");
        pass_count++;
    } else {
        printf("[FAIL] Bai tap 2: w1=%zu(3), r1=%zu(2), w2=%zu(4), w3=%zu(0), r2=%zu(5), seq=%d\n",
               w1, r1, w2, w3, r2, seq2);
    }

    /* --- Test Bài tập 3: Fixed-Frame 4-Byte FSM --- */
    FixedFSM_t fsm3;
    FSM_Init(&fsm3);
    Packet4B_t pkt3 = {0};

    const uint8_t stream3[] = {
        0x00U, 0x12U,                  // Rác trước Header
        0xAAU, 0x01U, 0x99U, 0x55U,    // Gói hợp lệ 1: cmd=0x01, data=0x99
        0xAAU, 0x02U, 0x88U, 0xFFU,    // Gói lỗi Footer (0xFF != 0x55)
        0xAAU, 0x03U, 0x77U, 0xAAU,    // Gói lỗi Footer nhưng byte cuối là 0xAA (Header gói kế!)
               0x04U, 0x66U, 0x55U     // Tiếp nối của gói hợp lệ 2: cmd=0x04, data=0x66
    };

    uint8_t valid3 = 0U;
    bool p1_ok = false, p2_ok = false;
    for (size_t i = 0U; i < sizeof(stream3); i++) {
        if (FSM_Feed(&fsm3, stream3[i], &pkt3)) {
            valid3++;
            if ((valid3 == 1U) && (pkt3.cmd == 0x01U) && (pkt3.data == 0x99U)) {
                p1_ok = true;
            }
            if ((valid3 == 2U) && (pkt3.cmd == 0x04U) && (pkt3.data == 0x66U)) {
                p2_ok = true;
            }
        }
    }

    if ((valid3 == 2U) && p1_ok && p2_ok) {
        printf("[PASS] Bai tap 3: FixedFSM giai ma goi 4 byte va phuc hoi khi loi Footer chinh xac!\n");
        pass_count++;
    } else {
        printf("[FAIL] Bai tap 3: valid3=%u (ky vong 2), p1_ok=%d, p2_ok=%d\n", valid3, p1_ok, p2_ok);
    }

    /* --- Test Bài tập 4: Ghép nối Ring Buffer + FSM --- */
    uint8_t mem4[16] = {0};
    SimpleRB_t rb4;
    FixedFSM_t fsm4;
    SRB_Init(&rb4, mem4, sizeof(mem4));
    FSM_Init(&fsm4);

    // Gửi chia làm 2 đợt: Đợt 1 bị cắt đôi gói tin thứ 2 ngay giữa chừng!
    const uint8_t chunk1[] = {0x99U, 0xAAU, 0x10U, 0x20U, 0x55U, 0xAAU, 0x30U};
    const uint8_t chunk2[] = {0x40U, 0x55U, 0xAAU, 0x50U, 0x60U, 0x55U};

    Packet4B_t last_pkt = {0};
    SRB_PushArray(&rb4, chunk1, sizeof(chunk1));
    uint32_t c1 = Stream_DrainAndParse(&rb4, &fsm4, &last_pkt); // Giải mã được 1 gói (0x10, 0x20)

    SRB_PushArray(&rb4, chunk2, sizeof(chunk2));
    uint32_t c2 = Stream_DrainAndParse(&rb4, &fsm4, &last_pkt); // Giải mã thêm 2 gói, gói cuối (0x50, 0x60)

    if ((c1 == 1U) && (c2 == 2U) && (last_pkt.cmd == 0x50U) && (last_pkt.data == 0x60U) && (SRB_Count(&rb4) == 0U)) {
        printf("[PASS] Bai tap 4: Stream_DrainAndParse ghep noi RingBuffer + FSM qua nhieu dot chinh xac!\n");
        pass_count++;
    } else {
        printf("[FAIL] Bai tap 4: c1=%u(1), c2=%u(2), last=(0x%02X, 0x%02X) ky vong (0x50, 0x60)\n",
               (unsigned int)c1, (unsigned int)c2, last_pkt.cmd, last_pkt.data);
    }

    printf("\n=== KET QUA TONG KET NGAY 09: %u / 4 BAI TAP DAT [PASS] ===\n", pass_count);
    return 0;
}