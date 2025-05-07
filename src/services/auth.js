import { API_CONFIG } from '../config';
import SparkMD5 from 'spark-md5';

const calculateMD5 = (str) => {
  const spark = new SparkMD5();
  spark.append(str);
  return spark.end();
};

export const loginUser = async (username, password) => {
  const response = await fetch(`${API_CONFIG.BASE_URL}${API_CONFIG.ENDPOINTS.LOGIN}`, {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json'
    },
    body: JSON.stringify({
      user: username,
      pwd: password
    })
  });

  const data = await response.json();
  if (data.code !== 0) {
    throw new Error(data.message || '登录失败');
  }
  return data;
};

export const registerUser = async (values) => {
  const response = await fetch(`${API_CONFIG.BASE_URL}${API_CONFIG.ENDPOINTS.REGISTER}`, {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json'
    },
    body: JSON.stringify({
      userName: values.username,
      firstPwd: calculateMD5(values.password),
      nickName: values.nickname,
      email: values.email,
      phone: values.phone
    })
  });

  const data = await response.json();
  if (data.code !== 0 && data.code !== 2) {
    throw new Error(data.message || '注册失败');
  }
  return data;
};