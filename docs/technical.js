/**
 * Malama Developer Documentation Interactions (technical.js)
 */

document.addEventListener('DOMContentLoaded', () => {
    // Smooth Scrolling Offset handling for fixed navbar on target anchors
    const techNavLinks = document.querySelectorAll('.nav-links a[href^="#"], .mobile-menu a[href^="#"]');

    techNavLinks.forEach(link => {
        link.addEventListener('click', (e) => {
            const targetId = link.getAttribute('href');
            if (targetId && targetId !== '#') {
                const targetElement = document.querySelector(targetId);
                if (targetElement) {
                    e.preventDefault();
                    if (window.location.hash !== targetId) {
                        history.pushState(null, '', targetId);
                    }
                    const navbarHeight = 70;
                    const elementPosition = targetElement.getBoundingClientRect().top;
                    const offsetPosition = elementPosition + window.pageYOffset - navbarHeight - 20;

                    window.scrollTo({
                        top: offsetPosition,
                        behavior: 'smooth'
                    });
                }
            }
        });
    });

    // Highlight active sidebar navigation based on scroll position
    const sections = document.querySelectorAll('section[id]');

    const highlightSectionOnScroll = () => {
        const scrollY = window.pageYOffset;

        sections.forEach(current => {
            const sectionHeight = current.offsetHeight;
            const sectionTop = current.offsetTop - 100;
            const sectionId = current.getAttribute('id');
            const navItem = document.querySelector(`.nav-links a[href*="#${sectionId}"]`);

            if (navItem) {
                if (scrollY > sectionTop && scrollY <= sectionTop + sectionHeight) {
                    navItem.style.color = 'var(--text-main)';
                    navItem.style.fontWeight = '700';
                } else {
                    navItem.style.color = '';
                    navItem.style.fontWeight = '';
                }
            }
        });
    };

    window.addEventListener('scroll', highlightSectionOnScroll);
    highlightSectionOnScroll();
});
