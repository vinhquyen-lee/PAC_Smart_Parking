import face_recognition
import numpy as np
from PIL import Image
import cv2 
from scipy.spatial.distance import cosine
from deepface import DeepFace


def extract_face_and_embedding(image_path, target_size=(160, 160)):
    image = face_recognition.load_image_file(image_path)
    
    face_locations = face_recognition.face_locations(image, model='hog')
    if len(face_locations) == 0:
        return image_path, None, None
    
    top, right, bottom, left = face_locations[0]

    # Crop để hiển thị
    face_crop = image[top:bottom, left:right]
    face_pil = Image.fromarray(face_crop).resize(target_size)

    # Không dùng CLAHE cho embedding
    encodings = face_recognition.face_encodings(
        image,
        known_face_locations=[(top, right, bottom, left)],
        num_jitters=1
    )

    if len(encodings) == 0:
        return image_path, face_pil, None

    return image_path, face_pil, encodings[0]

def cosine_similarity(emb1, emb2, threshold=0.91):
    if emb1 is None or emb2 is None:
        return False

    emb1 = np.array(emb1, dtype=np.float32)
    emb2 = np.array(emb2, dtype=np.float32)

    if emb1.shape != emb2.shape:
        print("Embedding shape mismatch:", emb1.shape, emb2.shape)
        return False, None

    dot = np.dot(emb1, emb2)
    norm = np.linalg.norm(emb1) * np.linalg.norm(emb2)

    if norm == 0:
        return False

    cosine_sim = dot / norm

    
    if cosine_sim < threshold:
        return False

    
    return True








