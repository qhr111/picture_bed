import React, { useState, useEffect } from 'react';
import { Table, Card, Tag, Button, message, Tooltip } from 'antd';
import { DownloadOutlined, SaveOutlined, ShareAltOutlined } from '@ant-design/icons';
import styled from '@emotion/styled';

const StyledCard = styled(Card)`
  .ant-table-thead > tr > th {
    background: #f7f7f7;
  }
  .download-count {
    font-weight: bold;
    color: #1890ff;
  }
`;

const TopDownloads = () => {
  const [topFiles, setTopFiles] = useState([]);
  const [loading, setLoading] = useState(false);

  useEffect(() => {
    fetchTopDownloads();
  }, []);

  const fetchTopDownloads = async () => {
    setLoading(true);
    try {
      // 这里替换为实际的 API 调用
      // const response = await getTopDownloadsApi();
      // setTopFiles(response.data);
      
      // 模拟数据
      setTopFiles([
        {
          id: '1',
          name: 'popular_image.jpg',
          downloads: 156,
          owner: 'user1',
          type: 'image',
          size: '2.5MB',
          url: 'https://picsum.photos/300/200',
        },
        {
          id: '2',
          name: 'document.pdf',
          downloads: 89,
          owner: 'user2',
          type: 'document',
          size: '1.2MB',
          url: 'https://example.com/doc.pdf',
        },
      ]);
    } catch (error) {
      message.error('获取下载榜失败！');
    } finally {
      setLoading(false);
    }
  };

  const handleDownload = async (file) => {
    try {
      const link = document.createElement('a');
      link.href = file.url;
      link.download = file.name;
      document.body.appendChild(link);
      link.click();
      document.body.removeChild(link);
      message.success('开始下载！');
    } catch (error) {
      message.error('下载失败！');
    }
  };

  const handleSave = async (fileId) => {
    try {
      // await saveToMineApi(fileId);
      message.success('转存成功！');
    } catch (error) {
      message.error('转存失败！');
    }
  };

  const columns = [
    {
      title: '排名',
      dataIndex: 'rank',
      key: 'rank',
      width: 80,
      render: (_, __, index) => (
        <Tag color={index < 3 ? 'gold' : 'default'}>
          {index + 1}
        </Tag>
      ),
    },
    {
      title: '文件名',
      dataIndex: 'name',
      key: 'name',
    },
    {
      title: '类型',
      dataIndex: 'type',
      key: 'type',
      width: 100,
      render: (type) => (
        <Tag color={type === 'image' ? 'blue' : 'green'}>
          {type}
        </Tag>
      ),
    },
    {
      title: '大小',
      dataIndex: 'size',
      key: 'size',
      width: 100,
    },
    {
      title: '上传者',
      dataIndex: 'owner',
      key: 'owner',
      width: 120,
    },
    {
      title: '下载次数',
      dataIndex: 'downloads',
      key: 'downloads',
      width: 120,
      render: (downloads) => (
        <span className="download-count">{downloads}</span>
      ),
      sorter: (a, b) => b.downloads - a.downloads,
      defaultSortOrder: 'descend',
    },
    {
      title: '操作',
      key: 'action',
      width: 200,
      render: (_, record) => (
        <Button.Group>
          <Tooltip title="下载">
            <Button 
              icon={<DownloadOutlined />}
              onClick={() => handleDownload(record)}
            />
          </Tooltip>
          <Tooltip title="转存">
            <Button 
              icon={<SaveOutlined />}
              onClick={() => handleSave(record.id)}
            />
          </Tooltip>
          <Tooltip title="分享">
            <Button 
              icon={<ShareAltOutlined />}
              onClick={() => {
                const shareLink = `${window.location.origin}/share/${record.id}`;
                navigator.clipboard.writeText(shareLink);
                message.success('分享链接已复制！');
              }}
            />
          </Tooltip>
        </Button.Group>
      ),
    },
  ];

  return (
    <StyledCard title="下载榜">
      <Table
        columns={columns}
        dataSource={topFiles}
        rowKey="id"
        loading={loading}
        pagination={{
          pageSize: 10,
          showTotal: (total) => `共 ${total} 个文件`,
        }}
      />
    </StyledCard>
  );
};

export default TopDownloads;