"use strict";

const NEEDS_REBASE_LABEL = 'status: needs rebase';
const AI_LABEL = 'AI';
const BASE_BRANCH = 'develop';
const AI_CO_AUTHORS = /\b(claude|codex|gemini|copilot|cursor|chatgpt)\b/i;

module.exports = async ({github, context}) => {
    const owner = context.payload.repository.owner.login;
    const repo = context.payload.repository.name;

    if (context.eventName === 'push') {
        await syncRebaseLabels(github, owner, repo);
        return;
    }

    const number = context.payload.pull_request.number;
    await syncPullRequestRebaseLabel(github, owner, repo, number);
    await addPullRequestAiLabel(github, owner, repo, context.payload.pull_request);
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

/**
 * Adds the 'AI' label if the pull request description or any of its commits is attributed to an AI agent.
 * The label is never removed, as it may also be applied manually.
 */
async function addPullRequestAiLabel(github, owner, repo, pull) {
    if (pull.labels.some(x => x.name === AI_LABEL)) {
        return;
    }

    const commits = await github.paginate(github.rest.pulls.listCommits, {owner, repo, pull_number: pull.number, per_page: 100});
    if (!hasAiAttribution(pull.body ?? '') && !commits.some(({commit}) => hasAiAttribution(commit.message))) {
        return;
    }

    console.log('Will add AI label');
    await github.rest.issues.addLabels({owner, repo, issue_number: pull.number, labels: [AI_LABEL]});
}

/**
 * Checks a commit message for explicit AI attribution: an 'Assisted-by' trailer, or a 'Co-authored-by' trailer naming a known AI agent.
 */
function hasAiAttribution(message) {
    return message.split(/\r?\n/).some(line => {
        const trailer = /^([\w-]+):\s*(\S.*)$/.exec(line);
        if (trailer === null) {
            return false;
        }

        const key = trailer[1].toLowerCase();
        return key === 'assisted-by' || (key === 'co-authored-by' && AI_CO_AUTHORS.test(trailer[2]));
    });
}
