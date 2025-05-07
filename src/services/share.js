import axios from 'axios';

const API_URL = process.env.REACT_APP_API_URL || 'http://localhost:3000';

export const getSharedFilesApi = async () => {
  const response = await axios.get(`${API_URL}/api/shared-files`);
  return response.data;
};

export const saveToMineApi = async (fileId) => {
  const response = await axios.post(`${API_URL}/api/files/${fileId}/save`);
  return response.data;
};

export const generateShareLinkApi = async (fileId) => {
  const response = await axios.post(`${API_URL}/api/files/${fileId}/share`);
  return response.data;
};

export const getTopDownloadsApi = async () => {
  const response = await axios.get(`${API_URL}/api/files/top-downloads`);
  return response.data;
};