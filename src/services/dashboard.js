import axios from 'axios';

const API_URL = process.env.REACT_APP_API_URL || 'http://localhost:3000';

export const getDashboardStatsApi = async () => {
  const response = await axios.get(`${API_URL}/api/dashboard/stats`);
  return response.data;
};

export const getRecentFilesApi = async () => {
  const response = await axios.get(`${API_URL}/api/dashboard/recent-files`);
  return response.data;
};