"""
Ratucho — gera os mapas do protótipo (L_MainMenu e L_Cozinha greybox).

Uso (com o editor):
  UnrealEditor.exe Ratucho.uproject -ExecutePythonScript="Tools/Unreal/build_levels.py"

Seguro por padrão: NÃO sobrescreve mapas existentes. Para regerar a cozinha,
defina a variável de ambiente RATUCHO_REBUILD=1 (apaga e recria L_Cozinha).

Escala: o rato tem ~68 cm; a cozinha é ~8x a real (bancada 7,2 m; mesa 6 m;
assento da cadeira 3,6 m). Unidades em cm, Z para cima, piso em Z=0.
"""
import math
import os
import unreal

LES = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
EAS = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
ATOOLS = unreal.AssetToolsHelpers.get_asset_tools()
EAL = unreal.EditorAssetLibrary

MENU_MAP = "/Game/Ratucho/Maps/L_MainMenu"
KITCHEN_MAP = "/Game/Ratucho/Maps/Levels/L_Cozinha"
MAT_DIR = "/Game/Ratucho/Materials/Instances"

CUBE = unreal.load_asset("/Engine/BasicShapes/Cube.Cube")
CYL = unreal.load_asset("/Engine/BasicShapes/Cylinder.Cylinder")
SPHERE = unreal.load_asset("/Engine/BasicShapes/Sphere.Sphere")
BASE_MAT = unreal.load_asset("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")


def log(msg):
    unreal.log("[RatuchoBuild] " + msg)


def rgb(r, g, b):
    """FColor é BGRA na memória: o construtor posicional do Python NÃO é (r, g, b).
    Sempre usar nomes, senão luzes quentes viram azuis."""
    return unreal.Color(r=r, g=g, b=b, a=255)


# ---------------------------------------------------------------------------
# Materiais (instâncias do material básico com parâmetro Color)
# ---------------------------------------------------------------------------
PALETTE = {
    "MI_GB_FloorTile": (0.93, 0.87, 0.76),
    "MI_GB_FloorTileDark": (0.78, 0.66, 0.52),
    "MI_GB_Wall": (0.96, 0.9, 0.78),
    "MI_GB_Baseboard": (0.98, 0.97, 0.95),
    "MI_GB_Cabinet": (0.36, 0.62, 0.64),
    "MI_GB_CabinetDoor": (0.42, 0.7, 0.72),
    "MI_GB_Countertop": (0.88, 0.86, 0.82),
    "MI_GB_Wood": (0.55, 0.34, 0.18),
    "MI_GB_WoodLight": (0.78, 0.56, 0.34),
    "MI_GB_Chair": (0.72, 0.38, 0.22),
    "MI_GB_Fridge": (0.93, 0.94, 0.96),
    "MI_GB_Metal": (0.62, 0.64, 0.68),
    "MI_GB_RedLabel": (0.82, 0.18, 0.14),
    "MI_GB_Rug": (0.22, 0.32, 0.58),
    "MI_GB_Cardboard": (0.78, 0.6, 0.38),
    "MI_GB_Jar": (0.62, 0.82, 0.78),
    "MI_GB_Green": (0.35, 0.62, 0.3),
    "MI_GB_Tomato": (0.9, 0.22, 0.12),
    "MI_GB_Handle": (0.3, 0.3, 0.32),
}
MATS = {}
MASTER_PATH = "/Game/Ratucho/Materials/Master/M_RatSolid"
# Rugosidade por material (madeira/metal/cerâmica/plástico com leitura diferente)
ROUGHNESS = {"MI_GB_Metal": 0.3, "MI_GB_Fridge": 0.25, "MI_GB_Jar": 0.15, "MI_GB_Countertop": 0.35,
             "MI_GB_Wood": 0.7, "MI_GB_WoodLight": 0.65, "MI_GB_Cardboard": 0.85, "MI_GB_Rug": 0.95}


def ensure_master():
    """Material-mestre de cor sólida (Color + Roughness), usado no greybox e nos ratos."""
    mel = unreal.MaterialEditingLibrary
    if EAL.does_asset_exist(MASTER_PATH):
        return unreal.load_asset(MASTER_PATH)
    mat = ATOOLS.create_asset("M_RatSolid", "/Game/Ratucho/Materials/Master", unreal.Material, unreal.MaterialFactoryNew())
    color = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -500, -100)
    color.set_editor_property("parameter_name", "Color")
    color.set_editor_property("default_value", unreal.LinearColor(0.8, 0.8, 0.8, 1.0))
    rough = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -500, 150)
    rough.set_editor_property("parameter_name", "Roughness")
    rough.set_editor_property("default_value", 0.6)
    emis = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -800, 300)
    emis.set_editor_property("parameter_name", "Emissive")
    emis.set_editor_property("default_value", 0.0)
    mul = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -300, 300)
    mel.connect_material_expressions(color, "", mul, "A")
    mel.connect_material_expressions(emis, "", mul, "B")
    mel.connect_material_property(color, "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.connect_material_property(mul, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.recompile_material(mat)
    EAL.save_loaded_asset(mat)
    log("M_RatSolid criado.")
    return mat


def ensure_materials():
    master = ensure_master()
    for name, rgb in PALETTE.items():
        path = MAT_DIR + "/" + name
        if EAL.does_asset_exist(path):
            mi = unreal.load_asset(path)
        else:
            mi = ATOOLS.create_asset(name, MAT_DIR, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        unreal.MaterialEditingLibrary.set_material_instance_parent(mi, master)
        unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(mi, "Color", unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1.0))
        unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(mi, "Roughness", ROUGHNESS.get(name, 0.6))
        EAL.save_loaded_asset(mi)
        MATS[name] = mi


# ---------------------------------------------------------------------------
# Helpers de construção
# ---------------------------------------------------------------------------
def _spawn_mesh(mesh, label, center, scale, mat, rot=(0, 0, 0), folder="Greybox", collision=True):
    actor = EAS.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*center), unreal.Rotator(rot[0], rot[1], rot[2]))
    actor.set_actor_label(label)
    actor.set_actor_scale3d(unreal.Vector(*scale))
    actor.set_folder_path(folder)
    smc = actor.static_mesh_component
    smc.set_static_mesh(mesh)
    smc.set_material(0, MATS[mat])
    if not collision:
        smc.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    return actor


def box(label, center, size, mat, rot=(0, 0, 0), folder="Greybox", collision=True):
    """center/size em cm. Rotator(roll, pitch, yaw) do Python = (X, Y, Z)."""
    return _spawn_mesh(CUBE, label, center, (size[0] / 100.0, size[1] / 100.0, size[2] / 100.0), mat, rot, folder, collision)


def cyl(label, center, diameter, height, mat, rot=(0, 0, 0), folder="Greybox", collision=True):
    return _spawn_mesh(CYL, label, center, (diameter / 100.0, diameter / 100.0, height / 100.0), mat, rot, folder, collision)


def climb(label, center, extent, yaw):
    """Volume de escalada. yaw: direção PARA FORA da superfície (0=+X, 90=+Y, 180=-X, 270=-Y)."""
    vol = EAS.spawn_actor_from_class(unreal.ClimbVolume, unreal.Vector(*center), unreal.Rotator(0, 0, yaw))
    vol.set_actor_label(label)
    vol.set_folder_path("Gameplay/Climb")
    vol.get_editor_property("box").set_box_extent(unreal.Vector(*extent))
    # Faixa visual colada à parede (mesma conta do OnConstruction em C++).
    marker = vol.get_editor_property("marker")
    marker.set_relative_location(unreal.Vector(-extent[0] + 1.0, 0, 0), False, False)
    marker.set_relative_rotation(unreal.Rotator(0, -90, 0), False, False)
    marker.set_relative_scale3d(unreal.Vector(extent[2] * 2.0 / 100.0, extent[1] * 2.0 / 100.0 * 0.35, 1.0))
    marker.set_material(0, MATS["MI_GB_Green"])
    return vol


def nav_link(label, bottom, top):
    """Link de navegação (bots) entre o chão diante da escalada e o topo da plataforma."""
    proxy = EAS.spawn_actor_from_class(unreal.NavLinkProxy, unreal.Vector(*bottom), unreal.Rotator(0, 0, 0))
    proxy.set_actor_label(label)
    proxy.set_folder_path("Gameplay/NavLinks")
    link = unreal.NavigationLink()
    link.set_editor_property("left", unreal.Vector(0, 0, 0))
    link.set_editor_property("right", unreal.Vector(top[0] - bottom[0], top[1] - bottom[1], top[2] - bottom[2]))
    link.set_editor_property("direction", unreal.NavLinkDirection.BOTH_WAYS)
    proxy.set_editor_property("point_links", [link])
    return proxy


def climb_with_link(label, face_point, normal_yaw, width, height, base_z=0.0):
    """Cria volume + navlink para uma face escalável.
    face_point: ponto (x, y) na superfície; normal_yaw: direção para fora."""
    nx, ny = math.cos(math.radians(normal_yaw)), math.sin(math.radians(normal_yaw))
    depth = 30.0
    cx, cy = face_point[0] + nx * depth, face_point[1] + ny * depth
    climb(label, (cx, cy, base_z + height / 2.0), (depth, width / 2.0, height / 2.0), normal_yaw)
    bottom = (face_point[0] + nx * 90.0, face_point[1] + ny * 90.0, base_z + 5.0)
    top = (face_point[0] - nx * 90.0, face_point[1] - ny * 90.0, base_z + height + 5.0)
    nav_link("NavLink_" + label, bottom, top)


def spawn_point(label, loc):
    a = EAS.spawn_actor_from_class(unreal.RatSpawnPoint, unreal.Vector(*loc), unreal.Rotator(0, 0, 0))
    a.set_actor_label(label)
    a.set_folder_path("Gameplay/RatSpawns")


def cheese_point(label, loc):
    a = EAS.spawn_actor_from_class(unreal.CheeseSpawnPoint, unreal.Vector(*loc), unreal.Rotator(0, 0, 0))
    a.set_actor_label(label)
    a.set_folder_path("Gameplay/CheeseSpawns")


def scale_brush_volume(actor, size):
    # Volumes nascem com brush de 200 uu; escala para o tamanho desejado.
    actor.set_actor_scale3d(unreal.Vector(size[0] / 200.0, size[1] / 200.0, size[2] / 200.0))


# ---------------------------------------------------------------------------
# Mapas
# ---------------------------------------------------------------------------
def build_menu():
    if EAL.does_asset_exist(MENU_MAP):
        log("L_MainMenu já existe — mantido.")
        return
    LES.new_level(MENU_MAP, False)
    sky = EAS.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 300), unreal.Rotator(0, 0, 0))
    sky.set_actor_label("SkyLight")
    LES.save_current_level()
    log("L_MainMenu criado.")


def build_kitchen():
    rebuild = os.environ.get("RATUCHO_REBUILD", "") == "1"
    if EAL.does_asset_exist(KITCHEN_MAP):
        if not rebuild:
            log("L_Cozinha já existe — mantido (defina RATUCHO_REBUILD=1 para regerar).")
            return
        # Regerar: abre o mapa e remove os atores gerados (o arquivo é mantido).
        if not LES.load_level(KITCHEN_MAP):
            raise RuntimeError("Não foi possível abrir L_Cozinha para regerar.")
        doomed = [a for a in EAS.get_all_level_actors()
                  if not isinstance(a, (unreal.WorldSettings, unreal.Brush)) or isinstance(a, unreal.NavMeshBoundsVolume)]
        EAS.destroy_actors(doomed)
        log("L_Cozinha limpo para regerar (%d atores removidos)." % len(doomed))
    else:
        if not LES.new_level(KITCHEN_MAP, False):
            raise RuntimeError("Falha ao criar L_Cozinha.")

    # --- Sala: 30 m x 24 m, paredes de 20 m --------------------------------
    box("Floor", (0, 0, -25), (3000, 2400, 50), "MI_GB_FloorTile", folder="Greybox/Room")
    # Faixas de piso mais escuro (leitura de escala/movimento)
    for i in range(-7, 8, 2):
        box("FloorStripe_%d" % i, (i * 200, 0, 0.3), (200, 2400, 0.6), "MI_GB_FloorTileDark", folder="Greybox/Room", collision=False)
    box("Wall_N", (1520, 0, 1000), (40, 2480, 2000), "MI_GB_Wall", folder="Greybox/Room")
    box("Wall_S", (-1520, 0, 1000), (40, 2480, 2000), "MI_GB_Wall", folder="Greybox/Room")
    box("Wall_E", (0, 1220, 1000), (3080, 40, 2000), "MI_GB_Wall", folder="Greybox/Room")
    box("Wall_W", (0, -1220, 1000), (3080, 40, 2000), "MI_GB_Wall", folder="Greybox/Room")
    box("Baseboard_S", (-1490, 0, 40), (20, 2400, 80), "MI_GB_Baseboard", folder="Greybox/Room")
    box("Baseboard_E", (0, 1190, 40), (2960, 20, 80), "MI_GB_Baseboard", folder="Greybox/Room")
    box("Baseboard_W", (0, -1190, 40), (2960, 20, 80), "MI_GB_Baseboard", folder="Greybox/Room")

    # --- Bancada (armários inferiores) ao longo da parede norte ------------
    # Corpo: x 1020..1500, y -1180..500, topo em 720.
    box("Cabinet_Body", (1260, -340, 345), (480, 1680, 690), "MI_GB_Cabinet", folder="Greybox/Counter")
    box("Countertop", (1260, -340, 705), (480, 1680, 30), "MI_GB_Countertop", folder="Greybox/Counter")
    # Portas/gavetas (frentes decorativas, rente à face)
    for i, y in enumerate([-1000, -620, -240, 140, 400]):
        box("Cabinet_Door_%d" % i, (1018, y, 330), (4, 330, 540), "MI_GB_CabinetDoor", folder="Greybox/Counter", collision=False)
        box("Drawer_%d" % i, (1018, y, 640), (4, 330, 90), "MI_GB_CabinetDoor", folder="Greybox/Counter", collision=False)
    # Escalada na frente da bancada (2 rotas)
    climb_with_link("Climb_Counter_W", (1020, -820), 180, 200, 720)
    climb_with_link("Climb_Counter_E", (1020, 330), 180, 200, 720)
    # Tábua de cortar apoiada: rampa do chão até a bancada (rota sem escalar)
    run, rise = 1150.0, 720.0
    length = math.hypot(run, rise) + 40.0
    pitch = math.degrees(math.atan2(rise, run))
    box("Ramp_CuttingBoard", (1020 - run / 2.0, -1080, rise / 2.0 - 8), (length, 170, 22), "MI_GB_WoodLight", rot=(0, pitch, 0), folder="Greybox/Counter")
    # Objetos em cima da bancada (cobertura e rotas)
    box("Counter_Toaster", (1300, -560, 870), (300, 380, 300), "MI_GB_Metal", folder="Greybox/Counter")
    cyl("Counter_Jar_A", (1350, -150, 880), 180, 320, "MI_GB_Jar", folder="Greybox/Counter")
    cyl("Counter_Jar_B", (1380, 60, 840), 140, 240, "MI_GB_RedLabel", folder="Greybox/Counter")
    box("Counter_CerealBox", (1380, 300, 980), (120, 260, 520), "MI_GB_Cardboard", folder="Greybox/Counter")
    box("Counter_Book", (1200, -950, 760), (260, 200, 80), "MI_GB_Green", folder="Greybox/Counter")

    # --- Geladeira (canto NE) e lixeira -------------------------------------
    box("Fridge", (1200, 980, 900), (520, 440, 1800), "MI_GB_Fridge", folder="Greybox/Appliances")
    box("Fridge_Handle", (930, 860, 900), (16, 20, 500), "MI_GB_Handle", folder="Greybox/Appliances")
    cyl("TrashBin", (700, 1020, 180), 260, 360, "MI_GB_Metal", folder="Greybox/Appliances")

    # --- Mesa (6 m x 4 m, tampo em 600) --------------------------------------
    tx, ty = -300.0, 0.0
    box("Table_Top", (tx, ty, 585), (1200, 800, 30), "MI_GB_Wood", folder="Greybox/Table")
    for sx in (-1, 1):
        for sy in (-1, 1):
            # Pernas quadradas (face plana = escalada estável), rente às bordas do tampo.
            box("Table_Leg_%d_%d" % (sx, sy), (tx + sx * 560, ty + sy * 360, 285), (80, 80, 570), "MI_GB_Wood", folder="Greybox/Table")
    # Escalada pelas pernas da mesa (lados leste e oeste)
    climb_with_link("Climb_TableLeg_E", (tx + 600, ty + 360), 0, 70, 600)
    climb_with_link("Climb_TableLeg_W", (tx - 600, ty - 360), 180, 70, 600)
    # Objetos sobre a mesa
    cyl("Table_Plate", (tx - 200, ty - 150, 610), 300, 18, "MI_GB_Baseboard", folder="Greybox/Table")
    cyl("Table_Mug", (tx + 300, ty + 180, 680), 130, 150, "MI_GB_RedLabel", folder="Greybox/Table")
    _spawn_mesh(SPHERE, "Table_Tomato", (tx + 50, ty + 250, 660), (1.1, 1.1, 1.0), "MI_GB_Tomato", folder="Greybox/Table")
    box("Table_Napkins", (tx - 420, ty + 230, 640), (160, 160, 80), "MI_GB_Cardboard", folder="Greybox/Table")

    # --- Cadeiras (assento em 360) ------------------------------------------
    for side, cy_ in (("N", 680.0), ("S", -680.0)):
        sgn = 1 if cy_ > 0 else -1
        box("Chair_%s_Seat" % side, (tx, cy_, 345), (380, 380, 30), "MI_GB_Chair", folder="Greybox/Chairs")
        for sx in (-1, 1):
            for sy in (-1, 1):
                cyl("Chair_%s_Leg_%d_%d" % (side, sx, sy), (tx + sx * 170, cy_ + sy * 170, 165), 40, 330, "MI_GB_Chair", folder="Greybox/Chairs")
        box("Chair_%s_Back" % side, (tx, cy_ + sgn * 175, 560), (380, 30, 400), "MI_GB_Chair", folder="Greybox/Chairs")
        # Painel lateral sob o assento (superfície sólida para escalar)
        box("Chair_%s_SidePanel" % side, (tx + 180, cy_, 165), (20, 340, 330), "MI_GB_Chair", folder="Greybox/Chairs")
        # Escalada pelo lado leste do assento
        climb_with_link("Climb_Chair_%s" % side, (tx + 190, cy_), 0, 160, 360)

    # --- Cobertura no chão -----------------------------------------------------
    cyl("FallenCan", (450, 300, 130), 260, 420, "MI_GB_RedLabel", rot=(90, 0, 0), folder="Greybox/Cover")
    box("Crate_A", (-1000, 700, 55), (220, 220, 110), "MI_GB_Cardboard", folder="Greybox/Cover")
    box("Crate_B", (-1150, 820, 165), (200, 200, 110), "MI_GB_Cardboard", folder="Greybox/Cover")
    box("CerealBox_Floor", (-900, -650, 180), (240, 100, 360), "MI_GB_Cardboard", rot=(0, 0, 25), folder="Greybox/Cover")
    cyl("Pan", (300, -700, 50), 320, 100, "MI_GB_Metal", folder="Greybox/Cover")
    cyl("Bottle", (-1200, -1000, 300), 120, 600, "MI_GB_Green", folder="Greybox/Cover")
    box("Rug", (-1150, 0, 1), (500, 1100, 2), "MI_GB_Rug", folder="Greybox/Cover")
    box("Shelf_Low", (-1380, -350, 120), (200, 500, 240), "MI_GB_WoodLight", folder="Greybox/Cover")
    climb_with_link("Climb_Shelf", (-1280, -350), 0, 200, 240)

    # --- Pontos de respawn (no chão e em plataformas) -------------------------
    spawns = [
        (-1300, -1000, 0), (-1300, 300, 0), (-1300, 1050, 0), (-700, -950, 0), (-650, 950, 0),
        (0, -1050, 0), (100, 1050, 0), (650, -350, 0), (700, 650, 0), (850, -1000, 0),
        (-750, 0, 0), (100, -150, 0), (1250, -150, 720), (tx + 250, ty - 150, 600),
    ]
    for i, s in enumerate(spawns):
        spawn_point("RatSpawn_%02d" % i, s)

    # --- Locais candidatos do queijo (todos alcançáveis) ----------------------
    cheeses = [
        (tx, ty, 0),                 # embaixo da mesa
        (1250, -300, 720),           # bancada
        (1200, 380, 720),            # bancada (leste)
        (tx - 350, ty + 100, 600),   # tampo da mesa
        (tx, 680, 360),              # assento da cadeira N
        (-1350, 1050, 0),            # canto SE
        (450, 1080, 0),              # perto da lixeira
        (-1380, -350, 240),          # prateleira baixa
        (300, -1050, 0),             # perto da tábua
    ]
    for i, c in enumerate(cheeses):
        cheese_point("CheeseSpawn_%02d" % i, c)

    # Fallback de PlayerStart (a regra real usa RatSpawnPoint).
    ps = EAS.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(-750, 0, 60), unreal.Rotator(0, 0, 0))
    ps.set_folder_path("Gameplay")

    # --- Navegação ------------------------------------------------------------
    nav = EAS.spawn_actor_from_class(unreal.NavMeshBoundsVolume, unreal.Vector(0, 0, 900), unreal.Rotator(0, 0, 0))
    nav.set_actor_label("NavMeshBounds")
    nav.set_folder_path("Gameplay")
    scale_brush_volume(nav, (3100, 2500, 2000))

    # --- Iluminação interna (sem Lumen): teto + lâmpada principal com sombra ---
    box("Ceiling", (0, 0, 2025), (3080, 2480, 50), "MI_GB_Wall", folder="Greybox/Room")
    lamp = EAS.spawn_actor_from_class(unreal.SpotLight, unreal.Vector(-100, 0, 1950), unreal.Rotator(0, -90, 0))
    lamp.set_actor_label("CeilingLamp_Key")
    lamp.set_folder_path("Lighting")
    lc = lamp.get_component_by_class(unreal.SpotLightComponent)
    lc.set_mobility(unreal.ComponentMobility.MOVABLE)
    lc.set_editor_property("intensity", 120000.0)
    lc.set_editor_property("attenuation_radius", 4500.0)
    lc.set_editor_property("outer_cone_angle", 70.0)
    lc.set_editor_property("inner_cone_angle", 35.0)
    lc.set_editor_property("light_color", rgb(255, 228, 190))
    lc.set_editor_property("cast_shadows", True)
    lc.set_editor_property("source_radius", 60.0)

    sky = EAS.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 2200), unreal.Rotator(0, 0, 0))
    sky.set_actor_label("SkyLight")
    sky.set_folder_path("Lighting")
    skc = sky.get_component_by_class(unreal.SkyLightComponent)
    skc.set_mobility(unreal.ComponentMobility.MOVABLE)
    # Ambiente interno neutro/quente: cubemap fixo em vez de capturar o céu azul.
    # Ambiente interno: o skylight captura a própria cozinha (paredes creme
    # iluminadas pela lâmpada) => luz ambiente quente e coerente, sem céu azul.
    skc.set_editor_property("real_time_capture", False)
    skc.set_editor_property("source_type", unreal.SkyLightSourceType.SLS_SPECIFIED_CUBEMAP)
    skc.set_editor_property("cubemap", unreal.load_asset("/Engine/EngineMaterials/DefaultCubemap.DefaultCubemap"))
    skc.set_editor_property("intensity", 1.2)
    skc.set_editor_property("light_color", rgb(255, 222, 188))
    skc.set_editor_property("lower_hemisphere_is_black", False)

    for i, (x, y) in enumerate([(-600, 0), (800, -300), (-900, 900), (600, 900)]):
        pl = EAS.spawn_actor_from_class(unreal.PointLight, unreal.Vector(x, y, 1500), unreal.Rotator(0, 0, 0))
        pl.set_actor_label("WarmFill_%d" % i)
        pl.set_folder_path("Lighting")
        plc = pl.get_component_by_class(unreal.PointLightComponent)
        plc.set_mobility(unreal.ComponentMobility.MOVABLE)
        plc.set_editor_property("intensity", 2500.0)
        plc.set_editor_property("attenuation_radius", 2600.0)
        plc.set_editor_property("cast_shadows", False)
        plc.set_editor_property("light_color", rgb(255, 214, 170))

    ppv = EAS.spawn_actor_from_class(unreal.PostProcessVolume, unreal.Vector(0, 0, 500), unreal.Rotator(0, 0, 0))
    ppv.set_actor_label("PostProcess_Global")
    ppv.set_folder_path("Lighting")
    ppv.set_editor_property("unbound", True)
    settings = ppv.get_editor_property("settings")
    settings.set_editor_property("override_auto_exposure_bias", True)
    settings.set_editor_property("auto_exposure_bias", 0.6)
    settings.set_editor_property("override_bloom_intensity", True)
    settings.set_editor_property("bloom_intensity", 0.4)
    # Cores um pouco mais vivas (sem mexer no balanço de branco: abaixo de 6500 K a Unreal ESFRIA a imagem).
    settings.set_editor_property("override_color_saturation", True)
    settings.set_editor_property("color_saturation", unreal.Vector4(1.12, 1.12, 1.12, 1.0))
    ppv.set_editor_property("settings", settings)

    LES.save_current_level()
    log("L_Cozinha criado e salvo.")


def main():
    ensure_materials()
    build_menu()
    build_kitchen()
    log("RATUCHO_BUILD_DONE")


main()
