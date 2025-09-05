#!/bin/bash

# this file is used to test the build-release.yml workflow using act
# get act from your favorite package manager https://nektosact.com/
# NOTE: it fails making the release because we don't provide GITHUB_TOKEN
# we don't want it actually pushing pages so don't provide it a key

act -W .github/workflows/build-release.yml -e act/events.json --artifact-server-path act/artifacts