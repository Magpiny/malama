/**
 * Malama — Interactive Web Workspace
 */

document.addEventListener('DOMContentLoaded', () => {
    // Mobile Navigation Menu Toggle
    const mobileToggle = document.getElementById('mobileToggle');
    const mobileMenu = document.getElementById('mobileMenu');

    if (mobileToggle && mobileMenu) {
        mobileToggle.addEventListener('click', () => {
            mobileMenu.classList.toggle('active');
        });

        document.querySelectorAll('.mobile-link').forEach(link => {
            link.addEventListener('click', () => {
                mobileMenu.classList.remove('active');
            });
        });
    }

    // Showcase Gallery Tabs
    const tabButtons = document.querySelectorAll('.tab-btn');
    const tabContents = document.querySelectorAll('.tab-content');

    tabButtons.forEach(btn => {
        btn.addEventListener('click', () => {
            const targetTab = btn.getAttribute('data-tab');

            tabButtons.forEach(b => b.classList.remove('active'));
            tabContents.forEach(c => c.classList.remove('active'));

            btn.classList.add('active');
            const targetContent = document.getElementById(targetTab);
            if (targetContent) {
                targetContent.classList.add('active');
            }
        });
    });

    // Installation Pathway Tabs
    const installButtons = document.querySelectorAll('.install-btn');
    const installPanels = document.querySelectorAll('.install-panel');

    installButtons.forEach(btn => {
        btn.addEventListener('click', () => {
            const targetInstall = btn.getAttribute('data-install');

            installButtons.forEach(b => b.classList.remove('active'));
            installPanels.forEach(p => p.classList.remove('active'));

            btn.classList.add('active');
            const targetPanel = document.getElementById(targetInstall);
            if (targetPanel) {
                targetPanel.classList.add('active');
            }
        });
    });

    // Code Block Copy Snippet Engine
    const copyButtons = document.querySelectorAll('.copy-code-btn');

    copyButtons.forEach(btn => {
        btn.addEventListener('click', () => {
            const codeBlock = btn.previousElementSibling;
            if (codeBlock) {
                const textToCopy = codeBlock.innerText;
                navigator.clipboard.writeText(textToCopy).then(() => {
                    const originalText = btn.innerText;
                    btn.innerText = 'Copied!';
                    btn.style.backgroundColor = '#c4929a';
                    setTimeout(() => {
                        btn.innerText = originalText;
                        btn.style.backgroundColor = '';
                    }, 2000);
                });
            }
        });
    });
});
