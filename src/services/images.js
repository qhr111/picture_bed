import { API_CONFIG } from '../config';
import SparkMD5 from 'spark-md5';

export const fetchUserImages = async (user) => {
  const response = await fetch(`${API_CONFIG.BASE_URL}${API_CONFIG.ENDPOINTS.MY_FILES}?cmd=normal`, {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json'
    },
    body: JSON.stringify({
      token: user.token,
      user: user.username,
      count: 20,
      start: 0
    })
  });

  const data = await response.json();
  if (data.code === 0) {
    return (data.files || []).map(file => ({
      ...file,
      name: file.filename,
      url: file.url.replace(API_CONFIG.STORAGE_URL, API_CONFIG.BASE_URL),
      pv: file.pv,
    }));
  }
  throw new Error(data.msg || '获取图片列表失败');
};

export const uploadImage = async (file, user) => {
  const formData = new FormData();
  formData.append('filename', file.name);
  formData.append('md5', await calculateMD5(file));
  formData.append('size', file.size);
  formData.append('file', file);
  formData.append('user', user.username);

  const response = await fetch(`${API_CONFIG.BASE_URL}${API_CONFIG.ENDPOINTS.UPLOAD}`, {
    method: 'POST',
    mode: 'no-cors',
    headers: {
      'Content-Type': 'application/octet-stream',
    },
    body: formData
  });

  const data = await response.json();
  if (data.code !== 0) {
    throw new Error(data.msg || '上传失败');
  }
  return data;
};

export const deleteImage = async (image, user) => {
  const response = await fetch(`${API_CONFIG.BASE_URL}${API_CONFIG.ENDPOINTSDEAL_FILE}?cmd=del`, {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json'
    },
    body: JSON.stringify({
      token: user.token,
      user: user.username,
      md5: image.md5,
      filename: image.file_name
    })
  });

  const data = await response.json();
  if (data.code !== 0) {
    throw new Error(data.msg || '删除失败');
  }
  return data;
};


const calculateMD5 = (file) => {
  return new Promise((resolve, reject) => {
    const reader = new FileReader();
    reader.onload = (e) => {
      const spark = new SparkMD5.ArrayBuffer();
      spark.append(e.target.result);
      const md5 = spark.end();
      resolve(md5);
    };
    reader.onerror = reject;
    reader.readAsArrayBuffer(file);
  });
};