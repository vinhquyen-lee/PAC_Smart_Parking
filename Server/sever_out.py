from flask import Flask, jsonify
from License_Plate import plate_ocr
from Face_Recognition import face_module
from Data_Processing.basic_processing import *
from firebase_admin import firestore
import time
import os
import uuid
import threading
import numpy as np
unique_id = 0
current_status = "IDLE"
# Initialize Firebase Storage
from datetime import datetime
bucket = init_firebase_storage()
db = init_firebase_db()
doc_ref = db.collection("stats").document("car_count")

LOCAL_DIR = "downloads"
os.makedirs(LOCAL_DIR, exist_ok=True)

PORT = 5000
app = Flask(__name__)


def download_all_images():
    images = []
    blobs = bucket.list_blobs(prefix='data/')

    for blob in blobs:
        # Bỏ split(), giữ nguyên blob.name
        file_name = blob.name.replace("data/", "")  

        if file_name == "":
            continue

        local_path = os.path.join(LOCAL_DIR, file_name)

        blob.download_to_filename(local_path)
        print(f"[FIREBASE] Đã tải: {blob.name}")

        # Lưu EXACT blob (không đổi tên)
        images.append((local_path, blob))

    return images

def get_face_plate(images_paths) :
    # f_img = face_embedding = []
    f_img = face_embedding = None
    p_img = p_path = p_ocr = None
    current_scores = 0.0
    for img_path in images_paths:
        full_img, face_img, embedding = face_module.extract_face_and_embedding(img_path)
        if face_img is not None:
            # f_img.append(full_img) 
            # face_embedding.append(embedding)
            f_img = full_img
            face_embedding = embedding
            break
            
    for img_path in images_paths:
        p_path, scores = plate_ocr.crop_license_plate_for_recognize(img_path)
        if p_path is not None : 
            if scores > current_scores : 
                p_ocr, p_img = plate_ocr.recognize_plate(p_path)
    # if p_ocr is None or len(face_embedding) == 0 :
    if p_ocr is None or face_embedding is None == 0 :
        return None, None, None, None
    
    return f_img, face_embedding, p_img, p_ocr


#Processing all images
def process_all_images(images):
    #images = download_all_images()
    if not images:
        return None, None, None, None
    
    local_paths = [local_path for local_path, _ in images]
    #local_paths = [local_path for local_path in images]
    f_img, face_embedding, p_img, p_ocr = get_face_plate(local_paths)
    #if len(f_img) == 0 or len(face_embedding) == 0 or p_img is None or p_ocr is None:
    if f_img is None  or face_embedding is None or p_img is None or p_ocr is None:
        return None, None, None, None
    return f_img, face_embedding, p_img, p_ocr
            

def verify_users(p_ocr, face_embedding, face_path, p_path) :
    
    users_ref = db.collection("Users")
    users = users_ref.stream()
    print(p_ocr)
    for user in users:
        user_id = user.id
        user_data = user.to_dict()
        plate_in_db = user_data.get("plate")  # Giả sử trong Firestore lưu trường 'plate'
        face_embedding_in_db = user_data.get("face_embedding")  # Lưu list hoặc numpy array
        face_embedding_in_db = np.array(face_embedding_in_db)
        if plate_in_db != p_ocr:
            continue
       # Tính similarity
        print("Có biển số xe trùng trong DB")
        is_same_person = face_module.cosine_similarity(
            face_embedding_in_db, face_embedding)

        # Lấy thông tin user in
        face_in = user_data.get("img_face")
        plate_in = user_data.get("img_plate")
        time_in = user_data.get("time")

        # Nếu đúng người → PASS
        if is_same_person is True:
            write_log_out(
                db, p_ocr, time_in, "PASS",
                face_in, plate_in, face_path, p_path
            )
            users_ref.document(user_id).delete()
            return True
        
        # Nếu biển số đúng nhưng mặt sai → FAIL ngay, không duyệt user khác
        write_log_out(db, p_ocr, time_in, "FAIL", 
                      face_in, plate_in, face_path, p_path)
        return False
    write_log_out(db, p_ocr, datetime.now().strftime("%Y-%m-%d %H:%M:%S"), "FAIL", 
                      "", "", face_path, p_path)
        
    # Không tìm thấy biển số trong DB
    return False



def programing_out() :
    global current_status, unique_id
    print("Tải ảnh")
    img = download_all_images()
    print("Xử lý tất cả ảnh")
    face_img, face_embedding, p_img, p_ocr = process_all_images(img)
    print(face_embedding, '\n', p_ocr)
    if face_img is None or face_embedding is None or p_img is None or p_ocr is None:
        current_status = "FAIL_IN"
        print("Không có biển số hoặc khuôn mặt")
        return "FAIL_IN"
    print("Có ảnh biển số")
    unique_id = uuid.uuid4()
    face_url = upload_image(bucket, face_img, unique_id, "face_out")
    plate_url = upload_image(bucket, p_img, unique_id, "plate_out")
    for local_path, blob in img:
        try:
            blob.delete()
            #xóa file local
            if os.path.exists(local_path):
                os.remove(local_path)
            print(f"Xóa: {blob.name}")
        except Exception as e:
            print(f"Lỗi xóa {blob.name}: {e}")
    result = verify_users(p_ocr, face_embedding, face_url, plate_url)

    if result is True :
        current_status = "PASS"
        print("Verify SUCCESS")
        return "PASS"

    print("verify FAIL")
    current_status = "FAIL_OUT"
    return "FAIL_OUT"

@app.route("/trigger-process", methods=["GET"])
def trigger_process():
    global current_status
    
    if current_status == "PROCESSING":  
        print("Đang xử lý")
        return "BUSY", 200 
        
    print("Bắt đầu chạy chương trình")
    current_status = "PROCESSING"
    
    thread = threading.Thread(target=programing_out)
    thread.start()
    
    return "STARTED", 200

@app.route("/check-status", methods=["GET"])
def check_status():
    global current_status
    return current_status, 200

@app.route("/", methods=["GET"])
def index():
    return "AIoT Server (Polling Mode) is running!", 200

if __name__ == "__main__":
    print(f"🚀 Server đang chạy trên 0.0.0.0:{PORT}")
    app.run(host="0.0.0.0", port=PORT, debug=True)

    




