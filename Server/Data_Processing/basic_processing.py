import firebase_admin
from firebase_admin import credentials, firestore, storage
import os
from datetime import datetime
import numpy as np

#Database 
def init_firebase_db():
    cred_db = credentials.Certificate("your_path_.json_database")
    
    # init app với tên "db_app"
    app_db = firebase_admin.initialize_app(cred_db, name="db_app")
    
    # connect Firestore với app_db
    db = firestore.client(app=app_db)
    return db

def init_firebase_storage():
    cred_storage = credentials.Certificate("your_path_.json_storage")
    app_storage = firebase_admin.initialize_app(
        cred_storage,
        {'storageBucket': "your bucket in storage"},
        name="storage_app"
    )

    bucket = storage.bucket(app=app_storage)
    return bucket


def load_users_Login(db):
    users_ref = db.collection("Users")
    docs = users_ref.stream()
    
    users = []
    for doc in docs:
        data = doc.to_dict()
        users.append({
            "id": doc.id,
            "plate_numbers": data.get("license_plate", []),
            "name": data.get("name")
        })
    return users
    
def upload_image(bucket, image_path, number, type):
    if not os.path.exists(image_path):
        raise FileNotFoundError("File không tồn tại: " + image_path)

    # tên file theo kiểu: in<number> image.jpg
    blob_name = f"image/{type}{number}.jpg"

    blob = bucket.blob(blob_name)
    blob.upload_from_filename(image_path)
    blob.make_public()

    print("Uploaded:", blob.public_url)
    return blob.public_url

def upload_image_array(bucket, image_data, number, img_type):
    from PIL import Image
    import numpy as np

    temp_path = f"temp_{img_type}_{number}.jpg"

    # Nếu là numpy array → convert thành PIL
    if isinstance(image_data, np.ndarray):
        img = Image.fromarray(image_data)
        img.save(temp_path)

    # Nếu là PIL Image → lưu trực tiếp
    elif isinstance(image_data, Image.Image):
        image_data.save(temp_path)

    else:
        raise TypeError(f"image_data phải là numpy array hoặc PIL Image, nhưng nhận: {type(image_data)}")

    # Upload lên Firebase
    blob_name = f"image/{img_type}{number}.jpg"
    blob = bucket.blob(blob_name)
    blob.upload_from_filename(temp_path)
    blob.make_public()

    # Xoá file tạm
    os.remove(temp_path)

    print("Uploaded:", blob.public_url)
    return blob.public_url


def write_log_in(db, face_embedding, plate, image_face, image_lp, time=None):
    if isinstance(face_embedding, np.ndarray):
        embedding_to_save = face_embedding.tolist()
    else:
        embedding_to_save = face_embedding # DeepFace trả về list nên chạy vào đây
    # --------------------
    log_data = {
        "face_embedding": embedding_to_save,
        "plate": plate,
        "time": time if time else datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
        "img_face": image_face,
        "img_plate": image_lp
    }

    db.collection("Users").add(log_data)
    print("Wrote log:", log_data)

def write_log_out(db, plate, time_in, caution, image_face, image_lp, image_face_out, image_lp_out):
    log_data = {
        "license_plate": plate,
        "time" : time_in, 
        "time_out": datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
        "caution" : caution ,
        "img_face": image_face,
        "img_plate" : image_lp, 
        "img_face_out": image_face_out,
        "img_plate_out" : image_lp_out
    }

    db.collection("Users_out").add(log_data)
    print("[LOG] Đã ghi log:", log_data)
