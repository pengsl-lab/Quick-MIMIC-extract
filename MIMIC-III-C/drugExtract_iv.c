/**
 * 提取相关信息
 */
void drug_row_info_extract(char *buff, struct DRUG *dr)
{
    // 初始化
    char STARTDATE[64];
    char ENDDATE[64];
    // FIXME: 有BUG, 原因未知
    dr->PROD_STRENGTH = (char *)malloc(256);

    unsigned int index = 0;
    int len = 0;
    // 读取STARTDATE
    buff_index_move(buff, &index, 3);
    str_cpy(buff, STARTDATE, &index, ',');
    // 读取ENDDATE
    index += 1;
    str_cpy(buff, ENDDATE, &index, ',');
    // 读取GSN
    // GSN字段可能为空
    buff_index_move(buff, &index, 3);
    dr->GSN = 0;
    while (buff[index] != ',')
    {
        dr->GSN = (dr->GSN * 10) + (buff[index] - '0');
        index++;
    }
    index += 1; // 跳过跳过,分隔符
    dr->NDC = 0;
    while (buff[index] != ',')
    {
        dr->NDC = (dr->NDC * 10) + (buff[index] - '0');
        index++;
    }
    // 读取PROD_STRENGTH
    index += 1; // 跳过残留的,分隔符
    str_cpy(buff, dr->PROD_STRENGTH, &index, ',');

    dr->STARTDATE = str_2_time_stamp(STARTDATE);
    dr->ENDDATE = str_2_time_stamp(ENDDATE);
    // 计算结束时间与开始时间之间的差值(小时)
    dr->DIFF = (int)((dr->ENDDATE - dr->STARTDATE) / 3600);
}

// 按开始时间从小到大排序
int drug_cmp(const void *a, const void *b)
{
    struct DRUG *pa = (struct DRUG *)a;
    struct DRUG *pb = (struct DRUG *)b;
    long long na = pa->STARTDATE;
    long long nb = pb->STARTDATE;
    return (int)(na - nb);
}

void drug_extract(int HADMID, struct DRUG **drug_rows, int *r_size, struct StaticInformation *sInfo)
{
    // 读处方信息表
    FILE *fptr = fopen(PRESCRIPTION, "r");
    const int MAX_BUFF_SIZE = 1024;
    char buff[MAX_BUFF_SIZE];

    const int ROWSMAXSIZE = 1024;
    unsigned int row_offset[ROWSMAXSIZE]; // 各条记录的偏移地址
    int drug_rows_size = 0;
    drug_rows_size = get_all_offset_m(HADMID, PRESCRIPTION_M_INDEX, row_offset, PRESCRIPTION_LEN);
    *drug_rows = (struct DRUG *)malloc(drug_rows_size * sizeof(struct DRUG));
    memset(*drug_rows, 0, sizeof(struct DRUG) * drug_rows_size);
    *r_size = 0;

    int i = 0;
    for (i = 0; i < drug_rows_size; i++)
    {
        fseek(fptr, row_offset[i], SEEK_SET);
        fgets(buff, MAX_BUFF_SIZE, fptr);
        drug_row_info_extract(buff, &(*drug_rows)[i]);
        // 存在无时间的数据, 需要跳过
        if ((*drug_rows)[i].STARTDATE < 0 || (*drug_rows)[i].ENDDATE < 0)
            continue;
        (*r_size)++;
        if ((*drug_rows)[i].STARTDATE < sInfo->begintime || sInfo->begintime == 0)
            sInfo->begintime = (*drug_rows)[i].STARTDATE;
        if ((*drug_rows)[i].ENDDATE > (*sInfo).endtime || sInfo->endtime == 0)
            sInfo->endtime = (*drug_rows)[i].ENDDATE;
    }
    // 按给药开始时间排序
    qsort(*drug_rows, drug_rows_size, sizeof((*drug_rows)[0]), drug_cmp);

    fclose(fptr);
}