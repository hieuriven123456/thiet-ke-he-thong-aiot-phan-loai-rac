# Smart Garbage Classification System using ESP32-CAM & Edge AI

## 📌 Giới thiệu

Đề tài xây dựng một **hệ thống thùng rác thông minh ứng dụng AIoT**, có khả năng **tự động nhận diện và phân loại rác** ngay tại nguồn, đồng thời **dự báo thời gian thùng rác đầy** nhằm hỗ trợ công tác thu gom hiệu quả hơn.

Hệ thống kết hợp **Edge AI (AI tại biên)** chạy trực tiếp trên **ESP32-CAM** để xử lý hình ảnh và **Machine Learning tại Server** do "ESP32 guử len để dự đoán thời gian đầy của thùng rác.

---

## 🎯 Mục tiêu đề tài

* Nhận diện và phân loại rác thành **rác phân hủy (PH)** và **rác không phân hủy (KPH)** bằng AI.
* Điều khiển **servo** để đưa rác vào đúng ngăn chứa.
* Theo dõi mức độ đầy của từng ngăn rác bằng **cảm biến siêu âm**.
* Dự báo **thời gian thùng rác đầy** bằng mô hình Machine Learning.
* Hiển thị thông tin trực quan trên **LCD**.

---

## 🧠 Công nghệ sử dụng

### Edge AI – Nhận diện rác

* **Nền tảng**: Edge Impulse
* **Mô hình**: FOMO (Fully Convolutional Neural Network)
* **Backbone**: MobileNetV2
* **Thiết bị**: ESP32-CAM, ESP32
* **Đầu vào**: Ảnh 96x96 từ camera
* **Đầu ra**: Phân loại rác PH / KPH theo từng vùng ảnh

👉 FOMO không sử dụng bounding box truyền thống mà chia ảnh thành các ô lưới, giúp giảm tài nguyên tính toán và phù hợp với thiết bị nhúng.

### Machine Learning – Dự báo rác đầy

* **Mô hình**: Random Forest
* **Nền tảng**: Flask Server (Python)
* **Dữ liệu đầu vào**: % rác đầy, tốc độ làm đầy, thời gian (giờ/ngày)
* **Đầu ra**: Thời điểm dự kiến thùng rác đầy (Time-to-full)

---

## ⚙️ Phần cứng sử dụng

* ESP32-CAM (xử lý AI hình ảnh)
* ESP32 (xử lý logic, giao tiếp server)
* Cảm biến siêu âm HY-SRF05 (đo mức rác)
* Động cơ Servo MG90S (phân loại rác)
* LCD 1602 (hiển thị thông tin)
* Module hạ áp LM2596

---

## 🔄 Nguyên lý hoạt động tổng thể

1. Camera ESP32-CAM chụp ảnh rác khi người dùng bỏ rác.
2. Mô hình FOMO chạy trực tiếp trên ESP32-CAM để phân loại rác.
3. Servo quay để đưa rác vào đúng ngăn (PH hoặc KPH).
4. Cảm biến siêu âm đo mức độ đầy của từng ngăn.
5. ESP32 gửi dữ liệu lên Flask Server qua HTTP (JSON).
6. Server dự báo thời gian thùng rác đầy và phản hồi kết quả.
7. Thông tin được hiển thị trên màn hình LCD.

---

## 📊 Đánh giá mô hình

### Mô hình Edge AI (FOMO)

* Accuracy ≥ 90%
* Precision, Recall, F1-score đều > 90%
* Thời gian suy luận ~685 ms
* Hoạt động ổn định trên ESP32-CAM

### Mô hình dự báo (Random Forest)

* R² ≈ 0.99
* MAE ≈ 0.0058
* RMSE thấp

---

## 🚀 Hướng phát triển

* Mở rộng phân loại rác chi tiết hơn (nhựa, kim loại, giấy,...)
* Tăng tập dữ liệu huấn luyện để cải thiện độ chính xác
* Ứng dụng LSTM cho dự báo dài hạn
* Kết nối LoRaWAN / NB-IoT cho triển khai diện rộng
* Tích hợp nền tảng quản lý Smart City

---

## 📎Video Demo


https://github.com/user-attachments/assets/604dd871-75b4-4a3d-a197-c069b6fa8048

https://github.com/user-attachments/assets/15a915bb-57ab-4b16-93ed-9f2817750e36

https://github.com/user-attachments/assets/d03b8f98-f206-4333-8095-0eddf8d65e95

https://github.com/user-attachments/assets/45fac97b-3201-4d8b-baba-329a2c11daad
---

## 📎 Ghi chú

* Mô hình AI được xuất từ Edge Impulse dưới dạng thư viện `.zip` và tích hợp trực tiếp vào Arduino IDE.
* Hệ thống được thiết kế với chi phí thấp, phù hợp cho mục đích nghiên cứu và thử nghiệm.

---

🌱 *Ứng dụng AIoT hướng tới môi trường xanh – sạch – bền vững.*
