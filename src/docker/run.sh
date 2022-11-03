#!/bin/bash

echo "95.215.16.242 mmo-gitlab-abp.int.flattr.net" >> /etc/hosts

gitlab-runner run --working-directory /opt/ci --user ci_user
