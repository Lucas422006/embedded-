#include <stdio.h>
#include <stdint.h>   // Thư viện bắt buộc để dùng uint8_t, uint32_t
#include <stdbool.h>  // Thư viện để dùng kiểu bool (true / false)

// 1. Dùng #define tạo các mặt nạ bit (Bitmask) giả lập thanh ghi điều khiển
#define CTRL_ENABLE_BIT     (1U << 0)  // Bit 0: Bật hệ thống (0x01)
#define CTRL_ALARM_BIT      (1U << 3)  // Bit 3: Bật còi báo động (0x08)

// 2. Dùng enum định nghĩa các trạng thái rõ ràng thay vì dùng số 0, 1, 2
typedef enum {
    SYS_OK = 0,
    SYS_WARNING,
    SYS_OVERTEMP
} System_Status_t;

// 3. Dùng struct gom nhóm dữ liệu của một node cảm biến
typedef struct {
    uint8_t         sensor_id;
    int16_t         temp_celsius;   // Nhiệt độ (có thể âm nên dùng int16_t)
    uint32_t        ctrl_reg;       // Thanh ghi điều khiển 32-bit giả lập
    System_Status_t status;
} SensorNode_t;

void Sensor_SetEnable(SensorNode_t *node, bool enable){
    if( node == NULL) return;

    if( enable == true){
        node->ctrl_reg|= CTRL_ENABLE_BIT;
    } else {
        node->ctrl_reg &= ~CTRL_ENABLE_BIT;
    }
}
// 4. Hàm nhận CON TRỎ (*node) để sửa trực tiếp dữ liệu bên trong biến gốc
void Sensor_Update(SensorNode_t *node, uint8_t high_byte, uint8_t low_byte) {
    if (node == NULL) {
        return; // Luôn kiểm tra con trỏ khác NULL trước khi truy cập
    }

    // Ghép 2 byte 8-bit từ cảm biến thành 1 số nguyên 16-bit bằng dịch bit (<<) và OR (|)
    int16_t raw_value = (int16_t)((high_byte << 8) | low_byte);
    node->temp_celsius = raw_value / 10; // Giả sử cảm biến trả về nhiệt độ nhân 10

    // Cấu trúc rẽ nhánh if - else kết hợp thao tác Bit trên thanh ghi ctrl_reg
    if (node->temp_celsius >= 60) {
        node->status = SYS_OVERTEMP;
        node->ctrl_reg |= CTRL_ALARM_BIT;   // SET bit 3 lên 1 (Bật còi)
    } else if (node->temp_celsius >= 45) {
        node->status = SYS_WARNING;
        node->ctrl_reg &= ~CTRL_ALARM_BIT;  // CLEAR bit 3 về 0 (Tắt còi)
    } else {
        node->status = SYS_OK;
        node->ctrl_reg &= ~CTRL_ALARM_BIT;  // CLEAR bit 3 về 0
    }
}

int main(void) {
    // Khởi tạo một biến cấu trúc cho cảm biến số 1
    SensorNode_t node1 = {
        .sensor_id    = 1,
        .temp_celsius = 25,
        .ctrl_reg     = CTRL_ENABLE_BIT, // Khởi đầu bật Bit 0
        .status       = SYS_OK
    };

    // Mảng dữ liệu giả lập 3 lần đọc cảm biến (Mỗi lần gồm High Byte và Low Byte)
    // Lần 1: 0x012C (300 -> 30 độ C) | Lần 2: 0x01F4 (500 -> 50 độ C) | Lần 3: 0x028A (650 -> 65 độ C)
    const uint8_t raw_stream[3][2] = {
        {0x01, 0x2C},
        {0x01, 0xF4},
        {0x02, 0x8A}
    };

    // Vòng lặp for duyệt qua 3 mẫu dữ liệu
    for (uint8_t i = 0; i < 3; i++) {
        // Truyền địa chỉ (&node1) vào hàm
        Sensor_Update(&node1, raw_stream[i][0], raw_stream[i][1]);

        printf("Mau %u | Nhiet do: %d C | Trang thai: %d | Thanh ghi CTRL: 0x%08X\n",
               i + 1, node1.temp_celsius, node1.status, node1.ctrl_reg);
    }

    return 0;
}