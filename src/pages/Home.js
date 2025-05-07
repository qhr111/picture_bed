import React, { useState, useEffect } from 'react';
import { Row, Col, Card, Statistic, List, Avatar, Button } from 'antd';
import { 
  CloudUploadOutlined, 
  FileOutlined, 
  DownloadOutlined,
  ShareAltOutlined 
} from '@ant-design/icons';
import styled from '@emotion/styled';

const StyledCard = styled(Card)`
  margin-bottom: 24px;
  .ant-card-head {
    border-bottom: none;
  }
`;

const Home = () => {
  const [stats, setStats] = useState({
    totalFiles: 0,
    totalDownloads: 0,
    totalShares: 0,
    storageUsed: '0 MB'
  });
  const [recentFiles, setRecentFiles] = useState([]);

  useEffect(() => {
    fetchDashboardData();
  }, []);

  const fetchDashboardData = async () => {
    try {
      // 这里替换为实际的 API 调用
      // const statsResponse = await getDashboardStatsApi();
      // const recentResponse = await getRecentFilesApi();
      // setStats(statsResponse.data);
      // setRecentFiles(recentResponse.data);

      // 模拟数据
      setStats({
        totalFiles: 42,
        totalDownloads: 156,
        totalShares: 23,
        storageUsed: '128 MB'
      });

      setRecentFiles([
        {
          id: '1',
          name: 'recent_image.jpg',
          type: 'image',
          uploadTime: '2024-01-20 14:30',
          size: '2.5MB',
          url: 'https://picsum.photos/50/50'
        },
        // ... 更多文件
      ]);
    } catch (error) {
      console.error('获取数据失败：', error);
    }
  };

  return (
    <div>
      <Row gutter={24}>
        <Col xs={24} sm={12} md={6}>
          <StyledCard>
            <Statistic
              title="总文件数"
              value={stats.totalFiles}
              prefix={<FileOutlined />}
            />
          </StyledCard>
        </Col>
        <Col xs={24} sm={12} md={6}>
          <StyledCard>
            <Statistic
              title="总下载次数"
              value={stats.totalDownloads}
              prefix={<DownloadOutlined />}
            />
          </StyledCard>
        </Col>
        <Col xs={24} sm={12} md={6}>
          <StyledCard>
            <Statistic
              title="已分享文件"
              value={stats.totalShares}
              prefix={<ShareAltOutlined />}
            />
          </StyledCard>
        </Col>
        <Col xs={24} sm={12} md={6}>
          <StyledCard>
            <Statistic
              title="存储空间使用"
              value={stats.storageUsed}
              prefix={<CloudUploadOutlined />}
            />
          </StyledCard>
        </Col>
      </Row>

      <StyledCard title="最近上传">
        <List
          itemLayout="horizontal"
          dataSource={recentFiles}
          renderItem={item => (
            <List.Item
              actions={[
                <Button type="link" icon={<DownloadOutlined />}>
                  下载
                </Button>,
                <Button type="link" icon={<ShareAltOutlined />}>
                  分享
                </Button>
              ]}
            >
              <List.Item.Meta
                avatar={<Avatar src={item.url} shape="square" />}
                title={item.name}
                description={`上传时间：${item.uploadTime} | 大小：${item.size}`}
              />
            </List.Item>
          )}
        />
      </StyledCard>
    </div>
  );
};

export default Home;