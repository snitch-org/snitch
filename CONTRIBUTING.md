# Contributing to _snitch_

<!-- MarkdownTOC autolink="true" -->

- [Who can contribute](#who-can-contribute)
    - [External contributors](#external-contributors)
    - [The team](#the-team)
        - [Maintainers](#maintainers)
        - [Mediators](#mediators)
- [Contributing a change](#contributing-a-change)
- [Reviewing a change](#reviewing-a-change)
    - [Conflict resolution \(technical\)](#conflict-resolution-technical)
    - [Conflict resolution \(personal\)](#conflict-resolution-personal)
- [Joining the team!](#joining-the-team)

<!-- /MarkdownTOC -->


## Who can contribute

### External contributors

Contributions to the source code are welcome from anyone, whether they are part of the [_snitch_ team](#the-team) or not! Simply follow the rules laid out below. If you are not familiar with contributing to an open-source project, feel free to open a [Discussion](https://github.com/snitch-org/snitch/discussions/categories/ideas) and ask for guidance. Regardless, you will receive help all the way, and particularly during the code review.


### The team

#### Maintainers

Maintainers are able to review and approve changes, as well as open and close issues. Although they have almost full write access to the repository, this does not include the _main_ branch, which always requires a Pull Request (PR) before any change can be merged into it. This means maintainers have to follow essentially the same procedure as external contributors when proposing a change. This ensures transparency.

Maintainers are also able (if they wish) to steer the project into new directions, propose features, refactoring, influence the roadmap, etc., and act on it. As described below, they have veto right on any change. They can also invite other maintainers to the team. Their only mandatory responsibility is to adhere to the [code of conduct](CODE_OF_CONDUCT.md).


#### Mediators

Mediators are a subset of the maintainers. They are responsible for settling disputes, be it technical or personal, and enforcing the [code of conduct](CODE_OF_CONDUCT.md).


## Contributing a change

The process goes as follows, regardless of who you are:

 - If you are considering adding a feature from _Catch2_ that _snitch_ currently does not support, please check the [_Catch2_ support roadmap](doc/comparison_catch2.md) first.

 - Please check the [Issue Tracker](https://github.com/snitch-org/snitch/issues) for any issue (open or closed) related to the feature you would like to add, or the problem you would like to solve. Read the discussion that has taken place there, if any, and check if any decision was taken that would be incompatible with your planned contribution.

 - If the change you plan to make is substantial, like a major refactor, or if it will affect the API of the library, you are strongly encouraged to first open an issue to describe the problem you are trying to solve, and how you plan to go about it. Achieving consensus early will make the actual review process much smoother, and save you time in the long run.

 - If the path is clear, create a temporary space to make your changes.

   - For external contributors: fork this repository and commit your changes on a new branch of the forked repository. This fork is only temporary; once your changes are (hopefully!) approved and merged, the fork can be deleted.

   - For maintainers: create a development branch directly in the _snitch_ repository, or use a fork if you prefer. Likewise, once your changes are approved and merged, the development branch can be deleted.

   In both cases, please use a new branch created from _main_ rather than pick up an older branch you created for a previous contribution. This will simplify the review process and allow your changes to be merged more quickly.

 - Try to use "atomic" commits (check that the code compiles before committing) and reasonably clear commit messages (no "WIP"). Linear history is preferred (i.e., avoid merge commits), but will not be enforced.

 - Check that your code mostly follows the [_snitch_ C++ Coding Guidelines](doc/coding_guidelines.md). If you are unsure, this will also be checked by reviewers during the review.

 - Run `clang-format` on your code before committing. The `.clang-format` file at the root of this repository contains all the formatting rules, and will be used automatically.

 - Add tests to cover your new code if applicable, then [run the _snitch_ tests](doc/testing_snitch.md) and fix any failure if you can.

 - Open a [Pull Request](https://github.com/snitch-org/snitch/pulls) (PR), with a description of what you are trying to do.

 - If there are issues you were unable to solve on your own (e.g., tests failing for reasons you do not understand, or high-impact design decisions), please feel free to open the pull request as a "draft", and highlight the areas that you need help with in the description. Once the issues are addressed, you can take your Pull Request out of draft mode.

 - Your code will then be [reviewed](#reviewing-a-change) by maintainers, and they may leave comments and suggestions. It is up to you to act on these comments and suggestions, and commit any required code changes. It is OK to push back on a suggestion if you have a good reason; don't always assume the reviewer is right.

 - Once the Pull Request is "approved", it will be merged and your changes will be directly incorporated into the _main_ branch. They will be available in the next release.

 - Job done! Congratulations.


## Reviewing a change

Everyone is welcome to review any proposed change, even if they are external contributors who were not involved in the change. However, only maintainers are allowed to approve or reject the change. Each maintainer who chooses to take part in the review has veto right: if they express a concern during the review, they can withhold their approval until this concern is addressed. The change will only be accepted once all involved reviewers are satisfied that their concerns have been addressed. The aim is to achieve consensus.


### Conflict resolution (technical)

In the unlikely event of a technical disagreement that cannot be resolved, either between the contributor and reviewer, or between multiple reviewers, mediators should be contacted for help. This can be done by opening a new [Mediation](https://github.com/snitch-org/snitch/discussions/categories/mediation) discussion.

Mediators will analyze the situation and the evidence, and decide on a course of action. Trivial matters may be settled directly by the mediator, although the decision they make can still be challenged by involving another mediator if necessary. For everything else, they will encourage all involved parties to find compromises and reach a consensus.

If the disagreement is still not resolved at this stage, the mediator will bring up the issue to other mediators. Mediators will attempt to reach their own consensus on the issue. If they cannot, they will organize a vote among all mediators, and settle the matter by simple majority (with project admins as tie breakers). Majority rule is a blunt tool, and it should be considered a last resort; consensus should always be the first aim.


### Conflict resolution (personal)

In case a disagreement degenerates into a personal conflict, a similar process will take place, this time within the framework defined in the [code of conduct](CODE_OF_CONDUCT.md). In particular, for conflict of a personal nature, please use the reporting method described in the code of conduct rather than publicly raise the problem in the [Mediation](https://github.com/snitch-org/snitch/discussions/categories/mediation) discussion. This will preserve your privacy and safety, since your report can remain anonymous. It will also avoid escalation from public shaming and finger pointing, which are not part of our conflict resolution process.


## Joining the team!

Do you like _snitch_ and want to contribute more frequently to it, maybe even influence design decisions for new features? You can request to become a maintainer. The process is open to anyone; you only have to successfully contribute one non-trivial change as an external contributor, following the instructions above (i.e., more than just fixing a typo in the readme, for example). Then, contact one of the maintainers to request to be included in the team. You should be accepted and on-boarded in the team immediately. Maintainers may also invite you to the team spontaneously if they value your contributions. You are always free to refuse this invitation and remain an external contributor, if that is your wish.

Joining the team as a maintainer comes with no responsibility to commit time or effort; however much you can give is enough. However, becoming a maintainer puts you in a position of leadership within the community, therefore you will be responsible for following the [code of conduct](CODE_OF_CONDUCT.md). Breaching this code may result in you being removed from the team.

Once you are part of the team as a maintainer, you can be nominated to become a mediator. Other mediators will review the application, and hopefully reach consensus on a decision. If no consensus is reached, the decision is made by majority vote.
