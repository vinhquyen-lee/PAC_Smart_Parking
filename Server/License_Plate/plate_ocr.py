import cv2
from ultralytics import YOLO
from pathlib import Path
import tempfile
# # Load models
lp_model = YOLO("Your path : lisence plate detect best.pt")     
ocr_model = YOLO("Your path : ocr best.pt")

# # Mapping class ID → ký tự
id2char = [
    "1","2","3","4","5","6","7","8","9","A",
    "B","C","D","E","F","G","H","K","L","M",
    "N","P","S","T","U","V","X","Y","Z","0"
]

def crop_license_plate_for_recognize(img_path, conf_thres=0.35, padding=10):
    img = cv2.imread(img_path)
    if img is None:
        print("[ERROR] Không đọc được ảnh:", img_path)
        return None

    # Dò biển số
    results = lp_model.predict(img, conf=conf_thres, verbose=False)
    r = results[0]

    if len(r.boxes) == 0:
        return None, None  # không phát hiện biển số

    # Lấy bbox confidence cao nhất
    boxes = r.boxes.xyxy.cpu().numpy()
    confs = r.boxes.conf.cpu().numpy()
    idx = confs.argmax()
    best_score = float(confs[idx]) 
    x1, y1, x2, y2 = boxes[idx]
    h, w = img.shape[:2]

    # padding
    x1 = max(0, int(x1) - padding)
    y1 = max(0, int(y1) - padding)
    x2 = min(w, int(x2) + padding)
    y2 = min(h, int(y2) + padding)

    crop = img[y1:y2, x1:x2]

    if crop.size == 0:
        return None, None

    # Lưu vào file tạm để dùng với recognize_plate
    tmp_file = Path(tempfile.gettempdir()) / "lp_crop.jpg"
    cv2.imwrite(str(tmp_file), crop)
    return str(tmp_file), best_score


#Trính xuất biển số xe
def recognize_plate(plate_img_path):
    img = cv2.imread(plate_img_path)
    results = ocr_model(img)[0]

    # Lưu thông tin ký tự: x_center, y_center, class_id
    chars = []
    for box in results.boxes:
        x1, y1, x2, y2 = map(int, box.xyxy[0])
        cls_id = int(box.cls[0])
        x_center = (x1 + x2) / 2
        y_center = (y1 + y2) / 2
        chars.append((x_center, y_center, cls_id))
    
    if not chars:
        return "", None  # không phát hiện ký tự

    # Tách 2 hàng bằng y_center
    y_centers = [c[1] for c in chars]
    mid_y = sum(y_centers) / len(y_centers)

    top_row = [c for c in chars if c[1] < mid_y]
    bottom_row = [c for c in chars if c[1] >= mid_y]

    # Sắp xếp theo x_center trong từng hàng
    top_row = sorted(top_row, key=lambda x: x[0])
    bottom_row = sorted(bottom_row, key=lambda x: x[0])

    # Ghép chuỗi ký tự: top row + bottom row
    plate_str = "".join([id2char[c[2]].upper() for c in top_row + bottom_row])
    return plate_str, plate_img_path








    