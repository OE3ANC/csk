""" 
使用说明 
    1. 确保已经安装python-gitlab: pip install python-gitlab
    2. 使用本脚本之前，请先获取到个人的token，注意保密,
    3. 获取token的方法：打开https://cloud.listenai.com/profile/personal_access_tokens，添加个人令牌
    4. 执行命令：python ./list_zephyr_downstream_mrs.py -r {仓库ID号} -b {分支名称} -t {个人的token}
例如
    python ./list_zephyr_downstream_mrs.py -r 1388 -b v3.4-csk-branch -t 1234567890abcdef 
"""

import gitlab
import os
import argparse
from datetime import datetime

def get_merge_request_list(token: str, output_path: str = './list_mr_info.MD', url: str = 'https://cloud.listenai.com/', repo_id=1388, branch="v3.4-csk-branch"):
    gl = gitlab.Gitlab(url, private_token=token)
    repo = gl.projects.get(repo_id)
    mrs = repo.mergerequests.list(
        state='merged', order_by='created_at', target_branch=branch)

    with open(output_path, 'w+') as fs:
        fs.write(f'# 合并请求详情\n\n')
        fs.write(f'仓库名称：[{repo.name}]({repo.http_url_to_repo})\n\n')
        fs.write(f'分支名称：{branch}\n\n')
        fs.write(f'截止时间：{ datetime.now()}\n\n')
        fs.write(f'合并数量：{len(mrs)}\n\n')

        for mr in mrs:
            fs.write(f'## [{mr.iid} {mr.title}]({mr.web_url})\n\n')
            fs.write(f'{mr.description}\n\n')


def do_arg_parse():
    parser = argparse.ArgumentParser(description='Scratch MRs and Parse')
    parser.add_argument('-r', '--repo', metavar='repo', required=False,
                        dest='repo', action='store',
                        help='gitlab repo id, exp: 1138')
    parser.add_argument('-b', '--branch', metavar='branch', required=True,
                        dest='branch', action='store',
                        help='specified branch, exp: master')
    parser.add_argument('-t', '--token', metavar='token', required=True,
                        dest='token', action='store',
                        help='repo private token')
    return parser.parse_args()


if __name__ == '__main__':
    args = do_arg_parse()
    get_merge_request_list(
        token=args.token, branch=args.branch, repo_id=args.repo)
    print('Done!')
