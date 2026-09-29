"use strict";

const NEEDS_REBASE_LABEL = 'status: needs rebase';
const BASE_BRANCH = 'develop';
const ISSUE_LABEL_GROUPS = [
    ['priority: critical', 'priority: high', 'priority: medium', 'priority: low'],
    ['complexity: high', 'complexity: medium', 'complexity: low'],
];

module.exports = async ({github, context}) => {
    const owner = context.payload.repository.owner.login;
    const repo = context.payload.repository.name;

    if (context.eventName === 'push') {
        await syncRebaseLabels(github, owner, repo);
        return;
    }

    const number = context.payload.pull_request.number;
    await syncPullRequestRebaseLabel(github, owner, repo, number);
    if (context.payload.action === 'opened' || context.payload.action === 'edited') {
        await addPullRequestIssueLabels(github, owner, repo, number);
    }
};

/**
 * Adds or removes the 'needs rebase' label depending on whether the pull request is mergeable without conflicts.
 * @returns {Promise<boolean>} A promise returning `true` if the check was successful, or `false` if the pull request should be checked again later.
 */
async function syncPullRequestRebaseLabel(github, owner, repo, number) {
    console.log(`Processing pull request #${number}`);

    const {data: pull} = await github.rest.pulls.get({owner, repo, pull_number: number});

    if (pull.mergeable === null) {
        console.log('Unknown mergeable status, skipping for now');
        return false;
    }

    const hasLabel = pull.labels.some(x => x.name === NEEDS_REBASE_LABEL);
    if (pull.mergeable === true && hasLabel) {
        console.log('Will remove label');
        await github.rest.issues.removeLabel({owner, repo, issue_number: number, name: NEEDS_REBASE_LABEL});
    }
    if (pull.mergeable === false && !hasLabel) {
        console.log('Will add label');
        await github.rest.issues.addLabels({owner, repo, issue_number: number, labels: [NEEDS_REBASE_LABEL]});
    }

    return true;
}

/**
 * Adds the highest priority and complexity labels of the issues linked to the pull request, unless it already has one.
 */
async function addPullRequestIssueLabels(github, owner, repo, number) {
    const {repository: {pullRequest: pull}} = await github.graphql(`
        query($owner: String!, $repo: String!, $number: Int!) {
            repository(owner: $owner, name: $repo) {
                pullRequest(number: $number) {
                    labels(first: 100) { nodes { name } }
                    closingIssuesReferences(first: 50) { nodes { labels(first: 100) { nodes { name } } } }
                }
            }
        }`, {owner, repo, number});

    const pullLabels = pull.labels.nodes.map(x => x.name);
    const issueLabels = pull.closingIssuesReferences.nodes.flatMap(issue => issue.labels.nodes.map(x => x.name));

    const labels = ISSUE_LABEL_GROUPS
        .filter(group => !group.some(label => pullLabels.includes(label)))
        .map(group => group.find(label => issueLabels.includes(label)))
        .filter(label => label !== undefined);
    if (labels.length > 0) {
        console.log(`Will add labels ${labels.join(', ')}`);
        await github.rest.issues.addLabels({owner, repo, issue_number: number, labels});
    }
}

const sleep = (ms) => new Promise((resolve) => setTimeout(resolve, ms));

/**
 * For each open pull request, adds or removes the 'needs rebase' label depending on whether the PR is mergeable without conflicts.
 * Handles retrying if the 'mergeable' status is unavailable
 */
async function syncRebaseLabels(github, owner, repo) {
    const iterator = github.paginate.iterator(github.rest.pulls.list, {
        owner,
        repo,
        state: 'open',
        base: BASE_BRANCH
    });
    const mergeablePending = [];
    for await (const response of iterator) {
        // The 'mergeable' property is not sent when using the 'List pull requests' method
        for (const number of response.data.map(pull => pull.number)) {
            if (!(await syncPullRequestRebaseLabel(github, owner, repo, number))) {
                mergeablePending.push(number);
            }
        }
    }

    for (const retryInterval of [5, 10, 20, 40, 80]) {
        if (mergeablePending.length === 0) {
            return;
        }

        console.log(`Waiting ${retryInterval}s before retrying ${mergeablePending.length} pull requests`);
        await sleep(retryInterval * 1000);

        for (let i = mergeablePending.length - 1; i >= 0; i--) {
            if (await syncPullRequestRebaseLabel(github, owner, repo, mergeablePending[i])) {
                mergeablePending.splice(i, 1);
            }
        }
    }
    throw new Error(
        "Not retrying anymore. It's likely that GitHub is having internal issues: check https://www.githubstatus.com."
    )
}