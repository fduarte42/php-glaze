(function () {
  var btn      = document.getElementById('menu-btn');
  var sidebar  = document.getElementById('sidebar');
  var backdrop = document.getElementById('sidebar-backdrop');

  if (!btn || !sidebar || !backdrop) return;

  function openNav() {
    sidebar.classList.add('open');
    backdrop.classList.add('visible');
    btn.setAttribute('aria-expanded', 'true');
    document.body.style.overflow = 'hidden';
  }

  function closeNav() {
    sidebar.classList.remove('open');
    backdrop.classList.remove('visible');
    btn.setAttribute('aria-expanded', 'false');
    document.body.style.overflow = '';
  }

  btn.addEventListener('click', function () {
    sidebar.classList.contains('open') ? closeNav() : openNav();
  });

  backdrop.addEventListener('click', closeNav);

  document.addEventListener('keydown', function (e) {
    if (e.key === 'Escape') closeNav();
  });
})();
