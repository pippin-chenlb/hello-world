void WifiApp::match_mac_and_send(std::vector<time_t> time_vt)
{
    //数据发送待定，先匹配找出符合要求的时间段内的mac数据
    //为减少遍历次数，优先将时间片段合并，只遍历本地缓冲一次
    std::vector<mac_match_time_t> time_vt_list;
    std::vector<int64_t> result_mac_vt; 
    int i,j;
    time_vt_list = merge_match_time(time_vt);
#if 1  //调试打印
    for (i = 0;i < time_vt_list.size();i++){
        printf("[%d] start[%lu]end[%lu]\n",i,time_vt_list[i].start,time_vt_list[i].end); 
    }
#endif
    for (i = 0;i < normal_mac_vt.size();i++){
        for(j = 0; j < time_vt_list.size();j++){
            if (normal_mac_vt[i].timestamp > time_vt_list[j].start
                && normal_mac_vt[i].timestamp < time_vt_list[j].end){
                break;
            }
        }
        if ( j < time_vt_list.size()){
            //满足条件，插入结果vector中
            int64_t mac64 = 0;
            wifi_mac_to_int64(normal_mac_vt[i].mac, &mac64);
            result_mac_vt.push_back(mac64);

            //note  结果上传待定，等待新协议制定
        }
    }
}

//合并需要匹配的时间段
std::vector<mac_match_time_t> WifiApp::merge_match_time(std::vector<time_t> time_vt)
{
    int i,j;
    //合并算法，时间起始点标记1，结束标记2
    struct point_t{
        time_t value;   //时间节点值
        int8_t flag;    //1/2两个值，1:start  2:end
    };
    struct point_t * point = NULL;
    int32_t pointNum = 0;
    std::vector<mac_match_time_t> vt;  

    pointNum = time_vt.size()*2; //一个时间点对应两个点
    point = (struct point_t *)malloc(sizeof(struct point_t)*pointNum);
    if (NULL == point){
        //error
        return vt;
    }
    for (i = 0 ; i < pointNum/2;i++){
        if (time_vt[i] - MATCH_BEFORE_TIME*60 < 0){
            point[i].value = 0;
        }else{
            point[i].value = time_vt[i] - MATCH_BEFORE_TIME*60;
        }
        point[i].flag = 1;
        point[i + pointNum/2].value = time_vt[i] + MATCH_AFTER_TIME*60;
        point[i + pointNum/2].flag = 2;
    }
    //散点信息排序
    struct point_t tmp;
    for (i = 0; i < pointNum; i++){
        for (j = i;j < pointNum; j++){
            if (point[i].value > point[j].value){
                tmp = point[i];
                point[i] = point[j];
                point[j] = tmp;
            }
        }
    }
    //节点整理为时间片段
    mac_match_time_t strMatchTime = {0};
    int32_t count = 0;
    for (i = 0; i < pointNum; i++){
        if (0 == count && (1 == point[i].flag)){
            strMatchTime.start = point[i].value;
            count++;
            continue;
        }
        if (2 == point[i].flag){
            count--;
        }else{
            count++;
        }
        if (0 == count){
            strMatchTime.end = point[i].value;
            vt.push_back(strMatchTime);
        }
    }
    free(point);
    return vt;
}
